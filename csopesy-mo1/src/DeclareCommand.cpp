#include "DeclareCommand.h"
#include "Process.h"

DeclareCommand::DeclareCommand(int pid, std::string var, std::uint16_t value)
    : ICommand(pid, DECLARE), var(std::move(var)), value(value) {}

void DeclareCommand::execute(Process& owner) {
    owner.getSymbolTable().setVariable(var, static_cast<int>(value));
}
