#include "Console.h"
#include "FCFSScheduler.h"
#include "RRScheduler.h"
#include "ScreenManager.h"
#include "MainMenuScreen.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <set>
#include <algorithm>

// Prepare a Windows console for UTF-8 + ANSI VT100 output; no-op everywhere else.
#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  static void enableAnsi() {
      SetConsoleOutputCP(CP_UTF8);
      SetConsoleCP(CP_UTF8);
      HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
      DWORD  m = 0;
      if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &m))
          SetConsoleMode(h, m | 0x0004 /*ENABLE_VIRTUAL_TERMINAL_PROCESSING*/);
  }
#else
  static void enableAnsi() {}
#endif

// ── ANSI escape codes ─────────────────────────────────────────────────────────
static const char* R  = "\033[0m";
static const char* B  = "\033[1m";
static const char* LG = "\033[92m";
static const char* CY = "\033[96m";
static const char* WH = "\033[97m";
static const char* GR = "\033[90m";
static const char* YL = "\033[93m";

namespace {

std::string rep(const char* s, int n) {
    std::string r;
    for (int i = 0; i < n; ++i) r += s;
    return r;
}

std::string fmtTime(std::time_t t) {
    if (t == 0) return "(N/A)";
    char buf[32];
    std::strftime(buf, sizeof(buf), "(%m/%d/%Y %I:%M:%S%p)", std::localtime(&t));
    return buf;
}

static const char* LOGO[] = {
    " ██████╗███████╗ ██████╗ ██████╗ ███████╗███████╗██╗   ██╗",
    "██╔════╝██╔════╝██╔═══██╗██╔══██╗██╔════╝██╔════╝╚██╗ ██╔╝",
    "██║     █████╗  ██║   ██║██████╔╝█████╗  ███████╗ ╚████╔╝ ",
    "██║     ╚════██╗██║   ██║██╔═══╝ ██╔══╝  ╚════██║  ╚██╔╝  ",
    "╚██████╗███████║╚██████╔╝██║     ███████╗███████║   ██║   ",
    " ╚═════╝╚══════╝ ╚═════╝ ╚═╝     ╚══════╝╚══════╝   ╚═╝   ",
    nullptr
};

void printBanner(const std::string& status = "awaiting initialize") {
#ifdef _WIN32
    system("cls");
#else
    std::cout << "\033[2J\033[H" << std::flush;
#endif

    for (int i = 0; LOGO[i]; ++i)
        std::cout << "  " << CY << LOGO[i] << R << "\n";

    std::cout << "\n";
    std::cout << "  " << GR << "Operating System Emulator  v1.0" << R << "\n";
    std::cout << "  " << GR << std::string(58, '-') << R << "\n";

    using Row = std::pair<const char*, std::string>;
    std::vector<Row> info = {
        { "OS    : ", "CSOPESY OS v1.0" },
        { "Shell : ", "csosh 1.0"       },
        { "Status: ", status            },
    };
    for (const auto& [label, value] : info)
        std::cout << "  " << CY << label << R << value << "\n";

    std::cout << "\n";
}

void printBootMessages() {
    struct Msg { int ms; const char* text; };
    static const Msg MSGS[] = {
        {  60, "Initialising kernel scheduler..."    },
        {  80, "Bringing up CPU subsystem..."        },
        {  50, "Starting dispatcher thread..."       },
        {  70, "Mounting virtual filesystem..."      },
        {  40, "Opening process log directory..."    },
        {  90, "Spawning console shell (csosh)..."   },
    };
    for (const auto& m : MSGS) {
        std::this_thread::sleep_for(std::chrono::milliseconds(m.ms));
        std::cout << "  " << GR << "[" << R
                  << B   << LG << "  OK  " << R
                  << GR  << "] " << R
                  << m.text << "\n";
    }
    std::cout << "\n" << B << LG
              << "  CSOPESY OS booted. Type 'initialize' to load config.txt.\n" << R << "\n";
}

} // namespace

// ── Console ───────────────────────────────────────────────────────────────────

Console::Console() {}

Console::~Console() {
    generating = false;
    if (genThread.joinable()) genThread.join();
}

void Console::run() {
    enableAnsi();
    printBanner();
    printBootMessages();

    // Hand input control to the screen multiplexer. The main menu is registered as a named screen
    // and run as the initial one; `screen -s/-r` push a ProcessScreen, `exit` quits.
    ScreenManager manager;
    manager.registerScreen(std::make_shared<MainMenuScreen>(*this));
    manager.run("main-menu");

    // Cleanup before returning to main.
    generating = false;
    if (genThread.joinable()) genThread.join();
    if (scheduler) scheduler->stop();
}

// ── Command handlers ──────────────────────────────────────────────────────────

void Console::cmdInitialize() {
    if (initialized) {
        std::cout << YL << "  Already initialized.\n" << R;
        return;
    }

    std::string err;
    if (!config.load("config.txt", err)) {
        std::cout << YL << "  initialize failed: " << R << err << "\n";
        return;
    }

    if (config.scheduler == SystemConfig::Scheduler::FCFS)
        scheduler = std::make_unique<FCFSScheduler>(config.numCpu, config.delaysPerExec);
    else
        scheduler = std::make_unique<RRScheduler>(
            config.numCpu, config.quantumCycles, config.delaysPerExec);

    scheduler->start();
    generator = std::make_unique<ProcessGenerator>(config);
    initialized = true;

    const char* policy = (config.scheduler == SystemConfig::Scheduler::FCFS) ? "FCFS" : "RR";
    std::string statusLine = "initialized — "
        + std::to_string(config.numCpu) + " core(s), scheduler=" + policy;
    printBanner(statusLine);
}

void Console::cmdSchedulerStart() {
    if (generating.load()) {
        std::cout << GR << "  scheduler-start: already running.\n" << R;
        return;
    }
    generating = true;
    genThread = std::thread([this] {
        // Spec: generate one process every `batch-process-freq` CPU cycles. Track the scheduler's
        // free-running CPU tick counter and admit a process each time it advances by
        // batchProcessFreq ticks. The first process is admitted unconditionally to bootstrap.
        std::uint64_t lastTick = scheduler->getCpuTick();
        bool bootstrap = true;
        while (generating.load()) {
            std::uint64_t now = scheduler->getCpuTick();
            if (bootstrap || now - lastTick >= config.batchProcessFreq) {
                bootstrap = false;
                lastTick = now;
                auto p = generator->generate();
                {
                    std::lock_guard<std::mutex> lk(registryMutex);
                    registry[p->getName()] = p;
                }
                scheduler->addProcess(p);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // poll; avoid busy-spin
        }
    });
    std::cout << GR << "  scheduler-start: batch generation started.\n" << R;
}

void Console::cmdSchedulerStop() {
    if (!generating.load()) {
        std::cout << GR << "  scheduler-stop: not currently running.\n" << R;
        return;
    }
    generating = false;
    if (genThread.joinable()) genThread.join();
    std::cout << GR << "  scheduler-stop: batch generation stopped.\n" << R;
}

void Console::cmdReportUtil() {
    std::ofstream file("csopesy-log.txt");
    if (!file) {
        std::cout << YL << "  report-util: could not write csopesy-log.txt\n" << R;
        return;
    }
    printProcessList(file, false);
    std::cout << GR << "  Report generated at " << R << "csopesy-log.txt\n";
}

std::shared_ptr<Process> Console::findProcess(const std::string& name) const {
    std::lock_guard<std::mutex> lk(registryMutex);
    auto it = registry.find(name);
    return it != registry.end() ? it->second : nullptr;
}

std::shared_ptr<Process> Console::getOrCreateProcess(const std::string& name) {
    {
        std::lock_guard<std::mutex> lk(registryMutex);
        auto it = registry.find(name);
        if (it != registry.end()) return it->second;  // `screen -s` attaches to an existing name
    }
    auto proc = generator->generate(name);
    {
        std::lock_guard<std::mutex> lk(registryMutex);
        registry[name] = proc;
    }
    scheduler->addProcess(proc);
    return proc;
}

void Console::printProcessList(std::ostream& os, bool color) const {
    // Color helper: returns the escape code when printing to terminal, else "".
    auto c = [color](const char* code) -> const char* {
        return color ? code : "";
    };

    auto running  = scheduler->getRunningProcesses();
    auto finished = scheduler->getFinishedProcesses();
    int  total    = scheduler->getNumCores();
    int  active   = scheduler->getActiveCores();
    int  utilPct  = (total > 0) ? (active * 100 / total) : 0;

    std::string hbar = rep("\xe2\x95\x90", 40);
    os << "\n"
       << c(B) << c(WH)
       << "  \xe2\x95\x94" << hbar << "\xe2\x95\x97\n"
       << "  \xe2\x95\x91   PROCESS SCHEDULER STATUS             \xe2\x95\x91\n"
       << "  \xe2\x95\x9a" << hbar << "\xe2\x95\x9d\n"
       << c(R) << "\n";

    os << "  " << c(CY) << "CPU Utilization : " << c(R)
       << c(B) << utilPct << "%" << c(R) << "\n"
       << "  " << c(CY) << "Cores Used      : " << c(R) << active << " / " << total << "\n"
       << "  " << c(CY) << "Cores Available : " << c(R) << (total - active) << "\n";

    os << "\n  " << c(B) << c(YL) << "Running Processes:" << c(R) << "\n"
       << "  " << c(GR) << std::string(64, '-') << c(R) << "\n";
    if (running.empty()) {
        os << c(GR) << "  (none)\n" << c(R);
    } else {
        for (const auto& p : running) {
            os << "  "
               << c(B) << c(LG) << std::left << std::setw(14) << p->getName() << c(R)
               << "  " << c(GR) << fmtTime(p->getStartTime()) << c(R)
               << "  Core:" << c(CY) << p->getCoreId() << c(R)
               << "  " << p->getCommandCounter() << " / " << p->getTotalCommands()
               << "\n";
        }
    }

    // Sleeping (WAITING) processes are off the cores and off the ready queue (parked on the
    // scheduler's waiting list), so they appear in neither running nor finished. Surface them by
    // scanning the registry for the WAITING state.
    std::vector<std::shared_ptr<Process>> sleeping;
    {
        std::lock_guard<std::mutex> lk(registryMutex);
        for (const auto& kv : registry)
            if (kv.second->getState() == Process::WAITING)
                sleeping.push_back(kv.second);
    }

    os << "\n  " << c(B) << c(CY) << "Sleeping Processes:" << c(R) << "\n"
       << "  " << c(GR) << std::string(64, '-') << c(R) << "\n";
    if (sleeping.empty()) {
        os << c(GR) << "  (none)\n" << c(R);
    } else {
        for (const auto& p : sleeping) {
            os << "  "
               << c(B) << c(YL) << std::left << std::setw(14) << p->getName() << c(R)
               << "  " << c(GR) << fmtTime(p->getStartTime()) << c(R)
               << "  " << c(YL) << "Sleeping" << c(R)
               << "  " << p->getCommandCounter() << " / " << p->getTotalCommands()
               << "\n";
        }
    }

    os << "\n  " << c(B) << c(WH) << "Finished Processes:" << c(R) << "\n"
       << "  " << c(GR) << std::string(64, '-') << c(R) << "\n";
    if (finished.empty()) {
        os << c(GR) << "  (none)\n" << c(R);
    } else {
        for (const auto& p : finished) {
            int tc = p->getTotalCommands();
            os << "  "
               << c(B) << std::left << std::setw(14) << p->getName() << c(R)
               << "  " << c(GR) << fmtTime(p->getFinishTime()) << c(R)
               << "  " << c(LG) << "Finished" << c(R)
               << "  " << tc << " / " << tc
               << "\n";
        }
    }
    os << "\n";
}
