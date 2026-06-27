#include "ForCommand.h"
#include "Process.h"
#include <memory>

namespace {
// Thin wrapper that delegates execute() to an inner command and appends an iteration tag to
// toString() — e.g. "ADD(x,y,1)  [FOR i=2/3]". Produced only by ForCommand::flatten(); never
// stored directly by user code.
struct AnnotatedCommand : ICommand {
    std::shared_ptr<ICommand> inner;
    std::string               tag;
    AnnotatedCommand(std::shared_ptr<ICommand> c, int pid, std::string t)
        : ICommand(pid, c->getCommandType()), inner(std::move(c)), tag(std::move(t)) {}
    void        execute(Process& p) override       { inner->execute(p); }
    std::string toString()          const override { return inner->toString() + tag; }
};
} // namespace

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
    std::string s = "FOR([";
    for (std::size_t i = 0; i < body.size(); ++i) {
        if (i) s += ", ";
        s += body[i]->toString();
    }
    s += "], " + std::to_string(repeats) + ")";
    return s;
}

std::vector<std::shared_ptr<ICommand>> ForCommand::flatten() const {
    std::vector<std::shared_ptr<ICommand>> out;
    out.reserve(body.size() * static_cast<std::size_t>(repeats));
    for (int i = 1; i <= repeats; ++i) {
        std::string tag = "  [FOR i=" + std::to_string(i) + "/" + std::to_string(repeats) + "]";
        for (auto& cmd : body)
            out.push_back(std::make_shared<AnnotatedCommand>(cmd, pid, tag));
    }
    return out;
}
