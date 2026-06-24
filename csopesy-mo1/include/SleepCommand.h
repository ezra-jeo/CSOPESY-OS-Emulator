#pragma once
#include "ICommand.h"
#include <cstdint>

// MO1 instruction: SLEEP(X)
// Sleeps the current process for X (uint8) CPU ticks and relinquishes the CPU.
// Under round-robin this lets another process run; the worker must yield, not block.
class SleepCommand : public ICommand {
public:
    SleepCommand(int pid, std::uint8_t ticks);
    void execute(Process& owner) override;

    std::uint8_t getTicks() const { return ticks; }

private:
    std::uint8_t ticks;
};
