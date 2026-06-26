#include "SubtractCommand.h"
#include "Process.h"
#include "Config.h"
#include <algorithm>

SubtractCommand::SubtractCommand(int pid, std::string dest, Operand lhs, Operand rhs)
    : ICommand(pid, SUBTRACT), dest(std::move(dest)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

void SubtractCommand::execute(Process& owner) {
    int result = static_cast<int>(lhs.resolve(owner)) - static_cast<int>(rhs.resolve(owner));
    result = std::max(result, 0); // underflow saturates to 0 per spec
    owner.getSymbolTable().setVariable(dest, result);

    if (Config::LOG_PER_COMMAND)
        owner.logMessage("SUBTRACT(" + dest + ", " + lhs.toString() + ", " + rhs.toString() +
                         ") => " + dest + " = " + std::to_string(result));
}
