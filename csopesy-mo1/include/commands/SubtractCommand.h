#pragma once
#include "ICommand.h"
#include "Operand.h"
#include <string>

// MO1 instruction: SUBTRACT(var1, var2/value, var3/value)
//   var1 = op2 - op3   (result clamped to uint16 [0, 65535]; underflow clamps to 0)
class SubtractCommand : public ICommand {
public:
    SubtractCommand(int pid, std::string dest, Operand lhs, Operand rhs);
    void execute(Process& owner) override;
    std::string toString() const override;

private:
    std::string dest;
    Operand     lhs;
    Operand     rhs;
};
