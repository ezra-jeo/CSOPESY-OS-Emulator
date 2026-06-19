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

// Enable ANSI VT100 on Windows 10+ consoles; no-op everywhere else.
#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  static void enableAnsi() {
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
static const char* MG = "\033[38;5;71m";    // Mint green  (#5faf5f)
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

// Pad a plain C-string to `width` visible characters (no ANSI inside s).
std::string pad(const char* s, int width) {
    std::string r(s);
    if ((int)r.size() < width) r.append(width - r.size(), ' ');
    return r;
}

// ── Linux Mint neofetch logo (13 lines) ───────────────────────────────────────
static const char* LOGO[] = {
    "MMMMMMMMMMMMMMMMMMMmmds+.        ",
    "MMm----::-://////////////oymNMd+`",
    "MMd      /++                -sNMb`",
    "MMNso/`  dMM    `.::-. .-::. .hMN`",
    "ddddMMh  dMM   :hNMNMNhNMNMNh: NMm",
    "    NMm  dMM  .NMN/-+MMM+-/NMN`dMM",
    "    NMm  dMM  -MMm  `MMM   dMM.dMM",
    "    NMm  dMM  -MMm  `MMM   dMM.dMM",
    "    NMm  dMM  .mmd`  mmm.  dMM.dMM",
    "    NMm  dMM`  ...`   ...  dMM.dMM",
    "    NMm  MMMMmmmmmmmmmmmmmmMMM..mMM",
    "    NMm  :hMMMMMMMMMMMMMMMMMd+  dMM",
    "    ...  `    -/oNMMMMMNo-`  `  ...",
    nullptr
};

void printBanner() {
    std::cout << "\033[2J\033[H"; // clear screen + cursor home

    // Build info column at runtime (avoids static array + std::string-from-null pitfall)
    std::ostringstream cores, procs, exec;
    cores << Config::NUM_CORES;
    procs << Config::NUM_PROCESSES << " x " << Config::PRINTS_PER_PROCESS << " instructions";
    exec  << Config::EXEC_DELAY_MS << " ms / instruction";

    // Each pair: { label (empty = special row), value }
    using Row = std::pair<std::string, std::string>;
    std::vector<Row> info = {
        { "",          std::string(B) + WH + "CSOPESY OS  v1.0" + R },
        { "__sep__",   ""                                            },
        { "OS:      ", "CSOPESY OS v1.0"                            },
        { "Shell:   ", "csosh 1.0"                                   },
        { "Cores:   ", cores.str()                                   },
        { "Procs:   ", procs.str()                                   },
        { "Sched:   ", "FCFS (non-preemptive)"                       },
        { "Delay:   ", exec.str()                                    },
        { "Logs:    ", Config::OUTPUT_DIR                            },
    };

    int logoRows = 0;
    while (LOGO[logoRows]) ++logoRows;
    int rows = std::max(logoRows, (int)info.size());

    for (int i = 0; i < rows; ++i) {
        // Logo column — fixed 38-char visible width
        if (i < logoRows)
            std::cout << "  " << MG << pad(LOGO[i], 38) << R;
        else
            std::cout << "  " << std::string(38, ' ');

        // Info column
        if (i < (int)info.size()) {
            std::cout << "  ";
            const auto& [label, value] = info[i];
            if (label.empty()) {                    // title row
                std::cout << value;                 // already has colour codes embedded
            } else if (label == "__sep__") {        // separator
                std::cout << GR << std::string(36, '-') << R;
            } else {                                // normal labelled row
                std::cout << CY << label << R << value;
            }
        }
        std::cout << "\n";
    }
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

        if (line == "exit") {
            std::cout << "\n" << GR << "  Shutting down CSOPESY OS...\n" << R << "\n";
            break;
        } else if (line == "screen -ls") {
            printProcessList();
        } else if (!line.empty()) {
            std::cout << GR << "csosh: " << R
                      << "command not found: " << YL << line << R << "\n";
        }
    }
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
