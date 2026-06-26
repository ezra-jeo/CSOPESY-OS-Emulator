#include "SleepCommand.h"
#include "Process.h"
#include "Config.h"

SleepCommand::SleepCommand(int pid, std::uint8_t ticks)
    : ICommand(pid, SLEEP), ticks(ticks) {}

void SleepCommand::execute(Process& owner) {
    owner.requestSleep(ticks);

    if (Config::LOG_PER_COMMAND)
        owner.logMessage("SLEEP(" + std::to_string(static_cast<int>(ticks)) + ")");
}
