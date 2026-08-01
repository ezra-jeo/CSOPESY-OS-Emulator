#include "AddCommand.h"
#include "Process.h"
#include <algorithm>

AddCommand::AddCommand(int pid, std::string dest, Operand lhs, Operand rhs)
    : ICommand(pid, ADD), dest(std::move(dest)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

void AddCommand::execute(Process& owner) {
    std::uint16_t a, b;
    if (!lhs.resolve(owner, a)) return;
    if (!rhs.resolve(owner, b)) return;
    int result = static_cast<int>(a) + static_cast<int>(b);
    result = std::min(result, 65535); // saturate at uint16 max
    owner.declareVar(dest, static_cast<std::uint16_t>(result));
}

std::string AddCommand::toString() const {
    return "ADD(" + dest + ", " + lhs.toString() + ", " + rhs.toString() + ")";
}
