#include "PrintCommand.h"
#include "Process.h"
#include "Config.h"
#include <ctime>
#include <thread>
#include <chrono>
#include <sstream>

// ── ICommand trivial definitions ─────────────────────────────────────────────
// (No separate ICommand.cpp in the project layout; these two belong here.)

ICommand::ICommand(int pid, CommandType commandType)
    : pid(pid), commandType(commandType) {}

ICommand::CommandType ICommand::getCommandType() { return commandType; }

// ── PrintCommand ─────────────────────────────────────────────────────────────

PrintCommand::PrintCommand(int pid, const std::string& toPrint)
    : ICommand(pid, PRINT), toPrint(toPrint) {}

// Fully implemented — appends one timestamped line to the process's log file,
// then sleeps EXEC_DELAY_MS so the run is observable.
void PrintCommand::execute(Process& owner) {
    // Build timestamp: (MM/DD/YYYY HH:MM:SSAM/PM)
    // Note: std::localtime is not thread-safe on all platforms; acceptable here
    // because each process's commands execute on exactly one worker at a time.
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "(%m/%d/%Y %I:%M:%S%p)", std::localtime(&t));

    // Spec format: <timestamp> Core:<id> "<message>"
    std::ostringstream oss;
    oss << buf << " Core:" << owner.getCoreId() << " \"" << toPrint << "\"";
    owner.log(oss.str());

    // Simulate instruction execution time so the scheduler is observable.
    std::this_thread::sleep_for(std::chrono::milliseconds(Config::EXEC_DELAY_MS));
}
