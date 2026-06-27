#include "ForCommand.h"
#include "Process.h"

ForCommand::ForCommand(int pid, std::vector<std::shared_ptr<ICommand>> body, int repeats)
    : ICommand(pid, FOR), body(std::move(body)), repeats(repeats) {}

void ForCommand::execute(Process& owner) {
    // The FOR node itself counts as one line in the process command counter.
    // Sub-instructions are executed inline here
    // Nesting cap (3 levels) is enforced at generation time, not here.
    for (int r = 0; r < repeats; ++r)
        for (auto& cmd : body)
            cmd->execute(owner);
}

std::uint32_t ForCommand::getInstructionCount() const {
    return static_cast<uint32_t>(body.size() * repeats);
}

std::string ForCommand::toString() const {
    // Fix to also print the commands inside (use individual to_string), better if shown the current iteration.
    std::string s = "FOR([";
    for (std::size_t i = 0; i < body.size(); ++i) {
        if (i) s += ", ";
        s += body[i]->toString();
    }
    s += "], " + std::to_string(repeats) + ")";
    return s;
}
