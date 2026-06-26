#include "PrintCommand.h"
#include "Process.h"
#include "Config.h"
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
    std::string message = toPrint;
    if (hasVar)
        message += std::to_string(owner.getSymbolTable().getVariable(varName));

    owner.logMessage(message);  // PRINT output is always logged (spec)
    std::this_thread::sleep_for(std::chrono::milliseconds(Config::EXEC_DELAY_MS));
}
