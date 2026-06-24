#include "ForCommand.h"
#include "Process.h"

ForCommand::ForCommand(int pid, std::vector<std::shared_ptr<ICommand>> body, int repeats)
    : ICommand(pid, FOR), body(std::move(body)), repeats(repeats) {}

void ForCommand::execute(Process& owner) {
    // TODO(student): run `body` `repeats` times by calling each sub-command's
    //   execute(owner). Decide how this interacts with the process command counter and
    //   per-instruction delay (delays-per-exec) so progress/preemption stay correct.
    (void)owner;
}
