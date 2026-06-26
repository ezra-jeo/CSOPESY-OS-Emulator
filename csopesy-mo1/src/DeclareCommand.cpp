#include "DeclareCommand.h"
#include "Process.h"
#include "Config.h"

DeclareCommand::DeclareCommand(int pid, std::string var, std::uint16_t value)
    : ICommand(pid, DECLARE), var(std::move(var)), value(value) {}

void DeclareCommand::execute(Process& owner) {
    owner.getSymbolTable().setVariable(var, static_cast<int>(value));

    if (Config::LOG_PER_COMMAND)
        owner.logMessage("DECLARE(" + var + ", " + std::to_string(value) + ")");
}
