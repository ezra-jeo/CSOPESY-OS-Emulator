#include "SleepCommand.h"
#include "Process.h"
#include <string>

SleepCommand::SleepCommand(int pid, std::uint8_t ticks)
    : ICommand(pid, SLEEP), ticks(ticks) {}

void SleepCommand::execute(Process& owner) {
    owner.requestSleep(ticks);
}

std::string SleepCommand::toString() const {
    return "SLEEP(" + std::to_string(static_cast<int>(ticks)) + ")";
}
