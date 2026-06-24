#include "ForCommand.h"
#include "Process.h"

ForCommand::ForCommand(int pid, std::vector<std::shared_ptr<ICommand>> body, int repeats)
    : ICommand(pid, FOR), body(std::move(body)), repeats(repeats) {}

void ForCommand::execute(Process& owner) {
    // The FOR node itself counts as one line in the process command counter.
    // Sub-instructions are executed inline here — they bring their own delays
    // (PRINT's EXEC_DELAY_MS, SLEEP's tick sleep, etc.).
    // Nesting cap (3 levels) is enforced at generation time, not here.
    for (int r = 0; r < repeats; ++r)
        for (auto& cmd : body)
            cmd->execute(owner);
}
