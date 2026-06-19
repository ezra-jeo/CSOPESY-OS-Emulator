#include "Console.h"
#include "Config.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace {
    // Format a time_t as (MM/DD/YYYY HH:MM:SSAM/PM).  Returns "(N/A)" for 0.
    std::string fmtTime(std::time_t t) {
        if (t == 0) return "(N/A)";
        char buf[32];
        std::strftime(buf, sizeof(buf), "(%m/%d/%Y %I:%M:%S%p)", std::localtime(&t));
        return buf;
    }
}

Console::Console(FCFSScheduler& scheduler) : scheduler(scheduler) {}

void Console::run() {
    std::cout << "CSOPESY OS Emulator — Phase 1 (FCFS)\n";
    std::cout << "Commands: screen -ls | exit\n\n";

    std::string line;
    while (true) {
        std::cout << "> ";
        std::cout.flush();

        if (!std::getline(std::cin, line)) break; // EOF (e.g. piped input)

        if (line == "exit") {
            break;
        } else if (line == "screen -ls") {
            printProcessList();
        } else if (!line.empty()) {
            std::cout << "Unknown command: \"" << line << "\"\n";
        }
    }
}

void Console::printProcessList() const {
    // Grab read-only snapshots under their respective locks.
    auto running  = scheduler.getRunningProcesses();
    auto finished = scheduler.getFinishedProcesses();
    int  total    = scheduler.getNumCores();
    int  active   = scheduler.getActiveCores();

    int utilPct = (total > 0) ? (active * 100 / total) : 0;

    std::cout << "\nCPU utilization: " << utilPct << "%\n";
    std::cout << "Cores used: "       << active           << "\n";
    std::cout << "Cores available: "  << (total - active) << "\n";

    // ── Running processes ────────────────────────────────────────────────────
    std::cout << "\nRunning processes:\n";
    if (running.empty()) {
        std::cout << "  (none)\n";
    } else {
        for (const auto& p : running) {
            std::cout << std::left  << std::setw(16) << p->getName()
                      << "  "      << std::setw(28) << fmtTime(p->getStartTime())
                      << "  Core:" << p->getCoreId()
                      << "  "      << p->getCommandCounter()
                      << " / "     << p->getTotalCommands()
                      << "\n";
        }
    }

    // ── Finished processes ───────────────────────────────────────────────────
    std::cout << "\nFinished processes:\n";
    if (finished.empty()) {
        std::cout << "  (none)\n";
    } else {
        for (const auto& p : finished) {
            int total_cmds = p->getTotalCommands();
            std::cout << std::left << std::setw(16) << p->getName()
                      << "  "     << std::setw(28) << fmtTime(p->getFinishTime())
                      << "  Finished"
                      << "  "     << total_cmds << " / " << total_cmds
                      << "\n";
        }
    }
    std::cout << "\n";
}
