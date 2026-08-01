#pragma once
#include "ICommand.h"
#include "Operand.h"
#include <cstdint>

// MO2 instruction: WRITE(memory_address, value)
// Writes a uint16 value — a literal or the current value of a variable — to the process's own
// address space at `addr`. `value` reuses Operand (same literal-or-variable operand type ADD/
// SUBTRACT already use) since the spec's own examples write both forms: WRITE 0x2000 42 and
// WRITE 0x500 varA.
class WriteCommand : public ICommand {
public:
    WriteCommand(int pid, std::uint16_t addr, Operand value);
    void execute(Process& owner) override;
    std::string toString() const override;

private:
    std::uint16_t addr;
    Operand       value;
};
