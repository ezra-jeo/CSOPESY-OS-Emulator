#include "SleepCommand.h"
#include "Process.h"

SleepCommand::SleepCommand(int pid, std::uint8_t ticks)
    : ICommand(pid, SLEEP), ticks(ticks) {}

void SleepCommand::execute(Process& owner) {
    owner.requestSleep(ticks);
}
