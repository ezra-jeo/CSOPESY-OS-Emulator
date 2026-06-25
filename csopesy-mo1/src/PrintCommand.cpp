#include "PrintCommand.h"
#include "Process.h"
#include "Config.h"
#include <ctime>
#include <sstream>
#include <thread>
#include <chrono>

// ── ICommand trivial definitions ─────────────────────────────────────────────
// (No separate ICommand.cpp in the project layout; these two belong here.)

ICommand::ICommand(int pid, CommandType commandType)
    : pid(pid), commandType(commandType) {}

ICommand::CommandType ICommand::getCommandType() { return commandType; }

// ── PrintCommand ─────────────────────────────────────────────────────────────

PrintCommand::PrintCommand(int pid, const std::string& toPrint)
    : ICommand(pid, PRINT), toPrint(toPrint) {}

PrintCommand::PrintCommand(int pid, std::string prefix, std::string varName)
    : ICommand(pid, PRINT), toPrint(std::move(prefix)),
      varName(std::move(varName)), hasVar(true) {}

void PrintCommand::execute(Process& owner) {
    // Timestamp: (MM/DD/YYYY HH:MM:SSAM/PM). std::localtime is not thread-safe in general, but
    // each process's commands run on exactly one worker at a time, so this is safe here.
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "(%m/%d/%Y %I:%M:%S%p)", std::localtime(&t));

    std::string message = toPrint;
    if (hasVar)
        message += std::to_string(owner.getSymbolTable().getVariable(varName));

    // Spec log format: <timestamp> Core:<id> "<message>"
    std::ostringstream oss;
    oss << buf << " Core:" << owner.getCoreId() << " \"" << message << "\"";
    owner.log(oss.str());

    std::this_thread::sleep_for(std::chrono::milliseconds(Config::EXEC_DELAY_MS));
}
