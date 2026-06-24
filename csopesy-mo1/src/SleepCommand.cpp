#include "SleepCommand.h"
#include "Process.h"
#include "Config.h"
#include <thread>
#include <chrono>

SleepCommand::SleepCommand(int pid, std::uint8_t ticks)
    : ICommand(pid, SLEEP), ticks(ticks) {}

void SleepCommand::execute(Process& owner) {
    // First-pass: busy-wait approximation — sleep the worker thread for
    // ticks * EXEC_DELAY_MS milliseconds. The process stays bound to its core
    // for the duration (no relinquish). A true tick-based CPU yield would
    // require a scheduler waiting-list (not yet implemented).
    (void)owner;
    std::this_thread::sleep_for(
        std::chrono::milliseconds(static_cast<int>(ticks) * Config::EXEC_DELAY_MS));
}
