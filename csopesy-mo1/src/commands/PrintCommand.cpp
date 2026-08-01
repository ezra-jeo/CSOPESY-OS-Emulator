#include "PrintCommand.h"
#include "Process.h"
#include <thread>

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
    std::uint16_t val = 0;
    if (hasVar) {
        if (!owner.readVar(varName, val)) return;   // fault set; do not log — CPUWorker restarts
    }

    std::string message = toPrint;
    if (hasVar) message += std::to_string(val);

    owner.logMessage(message);  // PRINT output is always logged (spec)
}

std::string PrintCommand::toString() const {
    if (hasVar)
        return "PRINT(\"" + toPrint + "\" + " + varName + ")";
    return "PRINT(\"" + toPrint + "\")";
}
