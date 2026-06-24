#include "SubtractCommand.h"
#include "Process.h"

SubtractCommand::SubtractCommand(int pid, std::string dest, Operand lhs, Operand rhs)
    : ICommand(pid, SUBTRACT), dest(std::move(dest)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

void SubtractCommand::execute(Process& owner) {
    // TODO(student): dest = lhs.resolve(owner) - rhs.resolve(owner), clamped to
    //   uint16 [0, 65535] (underflow saturates to 0), then store into the SymbolTable.
    (void)owner;
}
