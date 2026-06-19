#include "Config.h"
#include "Process.h"
#include "PrintCommand.h"
#include "FCFSScheduler.h"
#include "Console.h"
#include <iostream>
#include <memory>
#include <iomanip>
#include <sstream>

int main() {
    // ── Process factory ───────────────────────────────────────────────────────
    // Builds NUM_PROCESSES PCBs, each pre-loaded with PRINTS_PER_PROCESS
    // PrintCommands.  All processes are admitted to the scheduler before any
    // worker starts so the ready queue already reflects full arrival order.

    FCFSScheduler scheduler(Config::NUM_CORES);

    for (int i = 1; i <= Config::NUM_PROCESSES; ++i) {
        // Names: process_01, process_02, ..., process_10
        std::ostringstream oss;
        oss << Config::NAME_PREFIX << std::setw(2) << std::setfill('0') << i;
        const std::string name = oss.str();

        auto process = std::make_shared<Process>(i, name);

        // Load the instruction burst (all identical PRINT commands this phase).
        const std::string msg = "Hello world from " + name + "!";
        for (int j = 0; j < Config::PRINTS_PER_PROCESS; ++j)
            process->addCommand(std::make_shared<PrintCommand>(i, msg));

        // Long-term admission: push into the ready queue in arrival order.
        scheduler.addProcess(process);
    }

    // ── Start scheduler + worker threads ─────────────────────────────────────
    // One scheduler thread + Config::NUM_CORES worker threads are launched here.
    scheduler.start();

    // ── CLI (main thread) ─────────────────────────────────────────────────────
    // Console::run() blocks until the user types "exit".
    Console console(scheduler);
    console.run();

    // ── Clean shutdown ────────────────────────────────────────────────────────
    // Signals all threads to stop and joins them; open log files are flushed
    // and closed by ~Process() (ofstream destructor) when shared_ptrs drop to 0.
    scheduler.stop();

    std::cout << "Goodbye.\n";
    return 0;
}
