#include "DeclareCommand.h"
#include "Process.h"

DeclareCommand::DeclareCommand(int pid, std::string var, std::uint16_t value)
    : ICommand(pid, DECLARE), var(std::move(var)), value(value) {}

void DeclareCommand::execute(Process& owner) {
    // TODO(student): set `var` = `value` in owner.getSymbolTable().
    //   - SymbolTable currently stores int; uint16 clamping [0, 65535] still applies.
    //   - Optionally log a line so the action is visible inside the process screen.
    (void)owner;
}
