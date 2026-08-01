#include "SubtractCommand.h"
#include "Process.h"
#include <algorithm>

SubtractCommand::SubtractCommand(int pid, std::string dest, Operand lhs, Operand rhs)
    : ICommand(pid, SUBTRACT), dest(std::move(dest)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

void SubtractCommand::execute(Process& owner) {
    std::uint16_t a, b;
    if (!lhs.resolve(owner, a)) return;
    if (!rhs.resolve(owner, b)) return;
    int result = static_cast<int>(a) - static_cast<int>(b);
    result = std::max(result, 0); // underflow saturates to 0 per spec
    owner.declareVar(dest, static_cast<std::uint16_t>(result));
}

std::string SubtractCommand::toString() const {
    return "SUBTRACT(" + dest + ", " + lhs.toString() + ", " + rhs.toString() + ")";
}
