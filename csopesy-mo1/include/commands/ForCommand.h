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
    std::uint32_t getInstructionCount() const override; // body length * iteration
    std::string toString() const override;

    // Returns the body × repeats as a flat list of leaf commands, each annotated with their
    // iteration context in toString() (e.g. "ADD(x,y,1)  [FOR i=2/3]").
    // Used by ProcessGenerator so ForCommand participates in generation without being stored
    // in the commandList at runtime.
    std::vector<std::shared_ptr<ICommand>> flatten() const;

private:
    std::vector<std::shared_ptr<ICommand>> body;
    int                                    repeats;
};
