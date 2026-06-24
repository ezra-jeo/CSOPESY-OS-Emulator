#include "AddCommand.h"
#include "Process.h"

AddCommand::AddCommand(int pid, std::string dest, Operand lhs, Operand rhs)
    : ICommand(pid, ADD), dest(std::move(dest)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

void AddCommand::execute(Process& owner) {
    // TODO(student): dest = lhs.resolve(owner) + rhs.resolve(owner), clamped to
    //   uint16 [0, 65535], then store into owner.getSymbolTable().
    (void)owner;
}
