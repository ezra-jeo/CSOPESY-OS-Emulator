#pragma once
#include "ICommand.h"
#include <vector>
#include <memory>

// MO1 instruction: FOR([instructions], repeats)
// Runs a block of sub-instructions `repeats` times. Blocks may be nested, but the
// spec caps nesting at 3 levels — enforce that when GENERATING instructions, not here.
class ForCommand : public ICommand {
public:
    ForCommand(int pid, std::vector<std::shared_ptr<ICommand>> body, int repeats);
    void execute(Process& owner) override;
    std::string toString() const override;

private:
    std::vector<std::shared_ptr<ICommand>> body;
    int                                    repeats;
};
