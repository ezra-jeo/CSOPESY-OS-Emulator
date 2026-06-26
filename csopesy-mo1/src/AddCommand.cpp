#include "AddCommand.h"
#include "Process.h"
#include "Config.h"
#include <algorithm>

AddCommand::AddCommand(int pid, std::string dest, Operand lhs, Operand rhs)
    : ICommand(pid, ADD), dest(std::move(dest)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

void AddCommand::execute(Process& owner) {
    int result = static_cast<int>(lhs.resolve(owner)) + static_cast<int>(rhs.resolve(owner));
    result = std::min(result, 65535); // saturate at uint16 max
    owner.getSymbolTable().setVariable(dest, result);

    if (Config::LOG_PER_COMMAND)
        owner.logMessage("ADD(" + dest + ", " + lhs.toString() + ", " + rhs.toString() +
                         ") => " + dest + " = " + std::to_string(result));
}
