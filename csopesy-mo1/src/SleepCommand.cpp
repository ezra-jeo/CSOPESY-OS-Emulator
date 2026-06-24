#include "SleepCommand.h"
#include "Process.h"

SleepCommand::SleepCommand(int pid, std::uint8_t ticks)
    : ICommand(pid, SLEEP), ticks(ticks) {}

void SleepCommand::execute(Process& owner) {
    // TODO(student): mark the process WAITING for `ticks` CPU ticks and relinquish the
    //   core. Coordinate with the scheduler's tick counter so the process becomes READY
    //   again after `ticks` have elapsed. A simple first pass may just busy-wait/sleep,
    //   but the spec wants a real tick-based yield (important for round-robin).
    (void)owner;
}
