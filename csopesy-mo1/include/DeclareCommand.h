#pragma once
#include "ICommand.h"
#include <string>
#include <cstdint>

// MO1 instruction: DECLARE(var, value)
// Declares a uint16 variable in the owning process's SymbolTable and seeds it with
// `value`. Per spec, uint16 values are clamped to [0, 65535].
class DeclareCommand : public ICommand {
public:
    DeclareCommand(int pid, std::string var, std::uint16_t value);
    void execute(Process& owner) override;
    std::string toString() const override;

private:
    std::string   var;
    std::uint16_t value;
};
