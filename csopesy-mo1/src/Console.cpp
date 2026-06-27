#include "Console.h"
#include "FCFSScheduler.h"
#include "RRScheduler.h"
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

    std::string line;
    while (true) {
        std::cout << B << LG << "user@csopesy" << R
                  << ":"
                  << B << CY << "~" << R
                  << "$ ";
        std::cout.flush();

        if (!std::getline(std::cin, line)) break;

        auto args = tokenize(line);
        if (args.empty()) continue;
        if (!dispatch(args)) break;
    }

    // Cleanup before returning to main.
    generating = false;
    if (genThread.joinable()) genThread.join();
    if (scheduler) scheduler->stop();
}

// ── Command interpreter ───────────────────────────────────────────────────────

std::vector<std::string> Console::tokenize(const std::string& line) {
    std::vector<std::string> out;
    std::istringstream iss(line);
    for (std::string tok; iss >> tok; ) out.push_back(tok);
    return out;
}

bool Console::dispatch(const std::vector<std::string>& args) {
    const std::string& cmd = args[0];

    if (cmd == "exit") {
        std::cout << "\n" << GR << "  Shutting down CSOPESY OS...\n" << R << "\n";
        return false;
    }

    if (cmd == "initialize") { cmdInitialize(); return true; }

    // Recognize the set of post-init commands before checking the gate.
    static const std::set<std::string> known = {
        "screen", "scheduler-start", "scheduler-stop", "report-util"
    };

    if (!initialized) {
        std::cout << GR << "error: " << R << "run initialize first\n";
        return true;
    
    } else if (known.find(cmd) == known.end()) {
        std::cout << GR << "csosh: " << R
                  << "command not found: " << YL << cmd << R << "\n";
        return true;
    }

    if (cmd == "screen")          { cmdScreen(args);     return true; }
    if (cmd == "scheduler-start") { cmdSchedulerStart(); return true; }
    if (cmd == "scheduler-stop")  { cmdSchedulerStop();  return true; }
    if (cmd == "report-util")     { cmdReportUtil();     return true; }

    return true;
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

void Console::cmdScreen(const std::vector<std::string>& args) {
    if (args.size() >= 2 && args[1] == "-ls") {
        printProcessList(std::cout, true);
        return;
    }
    if (args.size() >= 3 && args[1] == "-s") { screenSession(args[2], false); return; }
    if (args.size() >= 3 && args[1] == "-r") { screenSession(args[2], true);  return; }

    std::cout << GR << "  usage: screen -ls | screen -s <name> | screen -r <name>\n" << R;
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

void Console::screenSession(const std::string& name, bool resume) {
    std::shared_ptr<Process> proc;
    {
        std::lock_guard<std::mutex> lk(registryMutex);
        auto it = registry.find(name);
        if (it != registry.end()) proc = it->second;
    }

    if (resume) {
        // -r: must exist and must not already be finished
        if (!proc || proc->isFinished()) {
            std::cout << "Process " << name << " not found.\n";
            return;
        }
    } else {
        // -s: create if it doesn't exist yet
        if (!proc) {
            proc = generator->generate(name);
            {
                std::lock_guard<std::mutex> lk(registryMutex);
                registry[name] = proc;
            }
            scheduler->addProcess(proc);
        }
    }

    // ── Attached sub-prompt ───────────────────────────────────────────────────
    auto showInfo = [&]() {
        std::cout << "\033[2J\033[H"; // clear screen
        std::cout << B << WH << "Process name: " << R << proc->getName() << "\n"
                  << B << WH << "ID: "           << R << proc->getPID()  << "\n\n";

        std::cout << B << WH << "Logs:\n" << R;
        for (const auto& entry : proc->getLogs())
            std::cout << entry << "\n";
        std::cout << "\n";

        if (proc->isFinished()) {
            std::cout << B << LG << "Finished!\n" << R;
        } else {
            std::cout << B << WH << "Current instruction line: " << R
                      << proc->getCommandCounter() << "\n"
                      << B << WH << "Lines of code: "            << R
                      << proc->getTotalCommands()  << "\n";
        }
        std::cout << "\n";

        // Full instruction listing (the process's program) with a pointer at the current line.
        // Keeps the spec's Logs/instruction-line fields above; this just makes the commands and
        // the current position visible.
        std::cout << B << WH << "Instructions:\n" << R;
        auto listing = proc->getInstructionListing();
        const int cur = proc->getCommandCounter(); // 1-based current line shown above
        for (std::size_t i = 0; i < listing.size(); ++i) {
            if (static_cast<int>(i + 1) == cur)
                std::cout << B << CY << "-> " << std::setw(2) << (i + 1) << ": "
                          << listing[i] << R << "\n";
            else
                std::cout << "   " << std::setw(2) << (i + 1) << ": " << listing[i] << "\n";
        }
        std::cout << "\n";
    };

    showInfo();

    std::string line;
    while (true) {
        std::cout << B << WH << "root:\\> " << R;
        std::cout.flush();

        if (!std::getline(std::cin, line)) break;

        auto toks = tokenize(line);
        if (toks.empty()) continue;

        if (toks[0] == "exit") break;

        if (toks[0] == "process-smi") {
            showInfo();
            continue;
        }

        std::cout << GR << "csosh: " << R
                  << "command not found: " << YL << toks[0] << R << "\n";
    }
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
