#include "PrintCommand.h"
#include "Process.h"
#include "Config.h"
#include <thread>
#include <chrono>

// ── ICommand trivial definitions ─────────────────────────────────────────────
// (No separate ICommand.cpp in the project layout; these two belong here.)

ICommand::ICommand(int pid, CommandType commandType)
    : pid(pid), commandType(commandType) {}

ICommand::CommandType ICommand::getCommandType() { return commandType; }

// ── PrintCommand ─────────────────────────────────────────────────────────────

PrintCommand::PrintCommand(int pid, const std::string& toPrint)
    : ICommand(pid, PRINT), toPrint(toPrint) {}

void PrintCommand::execute(Process& owner) {
    (void)owner;
    std::this_thread::sleep_for(std::chrono::milliseconds(Config::EXEC_DELAY_MS));
}
