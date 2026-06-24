#include "Console.h"
#include "Config.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <algorithm>

// Prepare a Windows console for UTF-8 + ANSI VT100 output; no-op everywhere else.
#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  static void enableAnsi() {
      // The logo and box-drawing chars are UTF-8 bytes. Without this the console
      // decodes them as CP-850/437 and prints mojibake (ΓòöΓòÉ...). 65001 = CP_UTF8.
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
static const char* R  = "\033[0m";          // reset
static const char* B  = "\033[1m";          // bold
static const char* LG = "\033[92m";         // bright green
static const char* CY = "\033[96m";         // cyan
static const char* WH = "\033[97m";         // bright white
static const char* GR = "\033[90m";         // dark gray
static const char* YL = "\033[93m";         // yellow

namespace {

// Repeat a UTF-8 string n times (needed for multi-byte box-drawing chars).
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

// ── CSOPESY splash logo — exact strings from BootSequence.cpp (src/shell/) ───
static const char* LOGO[] = {
    " ██████╗███████╗ ██████╗ ██████╗ ███████╗███████╗██╗   ██╗",
    "██╔════╝██╔════╝██╔═══██╗██╔══██╗██╔════╝██╔════╝╚██╗ ██╔╝",
    "██║     █████╗  ██║   ██║██████╔╝█████╗  ███████╗ ╚████╔╝ ",
    "██║     ╚════██╗██║   ██║██╔═══╝ ██╔══╝  ╚════██║  ╚██╔╝  ",
    "╚██████╗███████║╚██████╔╝██║     ███████╗███████║   ██║   ",
    " ╚═════╝╚══════╝ ╚═════╝ ╚═╝     ╚══════╝╚══════╝   ╚═╝   ",
    nullptr
};

void printBanner() {
    std::cout << "\033[2J\033[H"; // clear screen + cursor home

    // Print the CSOPESY logo centred, in the same cyan used by the ImGui splash.
    for (int i = 0; LOGO[i]; ++i)
        std::cout << "  " << CY << LOGO[i] << R << "\n";

    std::cout << "\n";

    // Subtitle line — matches "Operating System Emulator  v1.0" from BootSequence.cpp
    std::cout << "  " << GR << "Operating System Emulator  v1.0" << R << "\n";
    std::cout << "  " << GR << std::string(58, '-') << R << "\n";

    // System info panel
    std::ostringstream cores, procs, exec;
    cores << Config::NUM_CORES;
    procs << Config::NUM_PROCESSES << " x " << Config::PRINTS_PER_PROCESS << " instructions";
    exec  << Config::EXEC_DELAY_MS << " ms / instruction";

    using Row = std::pair<const char*, std::string>;
    std::vector<Row> info = {
        { "OS       : ", "CSOPESY OS v1.0"           },
        { "Shell    : ", "csosh 1.0"                  },
        { "Cores    : ", cores.str()                  },
        { "Processes: ", procs.str()                  },
        { "Scheduler: ", "FCFS (non-preemptive)"      },
        { "Exec delay: ", exec.str()                  },
        { "Log dir  : ", Config::OUTPUT_DIR           },
    };
    for (const auto& [label, value] : info)
        std::cout << "  " << CY << label << R << value << "\n";

    std::cout << "\n";
}

void printBootMessages() {
    struct Msg { int ms; const char* text; };
    static const Msg MSGS[] = {
        {  60, "Initialising kernel scheduler..."    },
        {  80, "Bringing up CPU cores (x4)..."       },
        {  50, "Starting FCFS dispatcher thread..."  },
        {  70, "Mounting virtual filesystem..."      },
        {  40, "Opening process log directory..."    },
        {  90, "Spawning core worker threads..."     },
        {  50, "Launching console shell (csosh)..."  },
    };
    for (const auto& m : MSGS) {
        std::this_thread::sleep_for(std::chrono::milliseconds(m.ms));
        std::cout << "  " << GR << "[" << R
                  << B   << LG << "  OK  " << R
                  << GR  << "] " << R
                  << m.text << "\n";
    }
    std::cout << "\n" << B << LG
              << "  CSOPESY OS booted successfully.\n" << R << "\n";
}

} // namespace

// ── Console ───────────────────────────────────────────────────────────────────

Console::Console(FCFSScheduler& scheduler) : scheduler(scheduler) {}

void Console::run() {
    enableAnsi();
    printBanner();
    printBootMessages();
    std::cout << GR << "  Type 'screen -ls' to list processes, 'exit' to quit.\n\n" << R;

    std::string line;
    while (true) {
        // Bash-style prompt: user@csopesy:~$
        std::cout << B << LG << "user@csopesy" << R
                  << ":"
                  << B << CY << "~" << R
                  << "$ ";
        std::cout.flush();

        if (!std::getline(std::cin, line)) break; // EOF / pipe closed

        auto args = tokenize(line);
        if (args.empty()) continue;
        if (!dispatch(args)) break; // dispatch() returns false on "exit"
    }
}

// ── Command interpreter ─────────────────────────────────────────────────────────

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

    // Per spec, every command except `initialize`/`exit` requires a loaded config.
    // TODO(student): once cmdInitialize() is implemented, gate the handlers below on
    //   `initialized` and print a "run initialize first" message when it is false.

    if (initialized) {
        if (cmd == "screen")          { cmdScreen(args);     return true; }
        if (cmd == "scheduler-start") { cmdSchedulerStart(); return true; }
        if (cmd == "scheduler-stop")  { cmdSchedulerStop();  return true; }
        if (cmd == "report-util")     { cmdReportUtil();     return true; }
    } else {
        std::cout << GR << "error: " << R
              << "run initialize first" << "\n";
    }

    std::cout << GR << "csosh: " << R
              << "command not found: " << YL << cmd << R << "\n";
    return true;
}

// ── Command handlers (MO1 stubs) ────────────────────────────────────────────────

void Console::cmdInitialize() {
    // TODO(student): load + validate config.txt, set `config`, and on success set
    //   initialized=true. Recreate/resize the scheduler to config.numCpu and pick the
    //   FCFS vs RR policy here. On failure, print the error from SystemConfig::load.
    std::string err;
    if (config.load("config.txt", err)) {
        initialized = true;

        
        std::cout << GR << "  Initialized from config.txt.\n" << R;
    } else {
        std::cout << YL << "  initialize failed: " << R << err << "\n";
    }
}

void Console::cmdScreen(const std::vector<std::string>& args) {
    // Recognise: screen -ls | screen -s <name> | screen -r <name>
    if (args.size() >= 2 && args[1] == "-ls") {
        printProcessList();
        return;
    }
    if (args.size() >= 3 && args[1] == "-s") { screenSession(args[2], /*resume=*/false); return; }
    if (args.size() >= 3 && args[1] == "-r") { screenSession(args[2], /*resume=*/true);  return; }

    std::cout << GR << "  usage: screen -ls | screen -s <name> | screen -r <name>\n" << R;
}

void Console::cmdSchedulerStart() {
    // TODO(student): begin generating dummy processes every config.batchProcessFreq CPU
    //   ticks (use ProcessGenerator) and admit them to the scheduler's ready queue.
    std::cout << GR << "  scheduler-start: not implemented yet.\n" << R;
}

void Console::cmdSchedulerStop() {
    // TODO(student): stop the batch generator started by scheduler-start.
    std::cout << GR << "  scheduler-stop: not implemented yet.\n" << R;
}

void Console::cmdReportUtil() {
    // TODO(student): write the same content as `screen -ls` (CPU util, cores used/avail,
    //   running + finished summaries) to csopesy-log.txt. Reuse printProcessList()'s
    //   formatting by factoring it to take an ostream&.
    std::cout << GR << "  report-util: not implemented yet.\n" << R;
}

void Console::screenSession(const std::string& name, bool resume) {
    // TODO(student): attached screen sub-prompt for process `name`.
    //   -s: create the process, attach, show its info (name, id, current/total line).
    //   -r: re-attach to an existing process; refuse if finished or not found.
    //   Inside the loop, support a process-smi-style info command + `exit` back to menu.
    (void)resume;
    std::cout << GR << "  screen session for '" << R << name << GR
              << "': not implemented yet.\n" << R;
}

void Console::printProcessList() const {
    auto running  = scheduler.getRunningProcesses();
    auto finished = scheduler.getFinishedProcesses();
    int  total    = scheduler.getNumCores();
    int  active   = scheduler.getActiveCores();
    int  utilPct  = (total > 0) ? (active * 100 / total) : 0;

    // ╔══...══╗  ╚══...══╝  using UTF-8 box-drawing chars
    std::string hbar = rep("\xe2\x95\x90", 40); // ═ repeated — must match content row width
    std::cout << "\n"
              << B << WH
              << "  \xe2\x95\x94" << hbar << "\xe2\x95\x97\n"   // ╔═...═╗
              << "  \xe2\x95\x91   PROCESS SCHEDULER STATUS             \xe2\x95\x91\n"
              << "  \xe2\x95\x9a" << hbar << "\xe2\x95\x9d\n"   // ╚═...═╝
              << R << "\n";

    // ── CPU stats ────────────────────────────────────────────────────────────
    std::cout << "  " << CY << "CPU Utilization : " << R
              << B << utilPct << "%" << R << "\n"
              << "  " << CY << "Cores Used      : " << R << active << " / " << total << "\n"
              << "  " << CY << "Cores Available : " << R << (total - active) << "\n";

    // ── Running processes ─────────────────────────────────────────────────────
    std::cout << "\n  " << B << YL << "Running Processes:" << R << "\n"
              << "  " << GR << std::string(64, '-') << R << "\n";
    if (running.empty()) {
        std::cout << GR << "  (none)\n" << R;
    } else {
        for (const auto& p : running) {
            std::cout << "  "
                      << B << LG << std::left << std::setw(14) << p->getName() << R
                      << "  " << GR << fmtTime(p->getStartTime()) << R
                      << "  Core:" << CY << p->getCoreId() << R
                      << "  " << p->getCommandCounter() << " / " << p->getTotalCommands()
                      << "\n";
        }
    }

    // ── Finished processes ────────────────────────────────────────────────────
    std::cout << "\n  " << B << WH << "Finished Processes:" << R << "\n"
              << "  " << GR << std::string(64, '-') << R << "\n";
    if (finished.empty()) {
        std::cout << GR << "  (none)\n" << R;
    } else {
        for (const auto& p : finished) {
            int tc = p->getTotalCommands();
            std::cout << "  "
                      << B << std::left << std::setw(14) << p->getName() << R
                      << "  " << GR << fmtTime(p->getFinishTime()) << R
                      << "  " << LG << "Finished" << R
                      << "  " << tc << " / " << tc
                      << "\n";
        }
    }
    std::cout << "\n";
}
