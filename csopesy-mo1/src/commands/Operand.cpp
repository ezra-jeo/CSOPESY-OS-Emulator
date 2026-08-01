#include "Operand.h"
#include "Process.h"

bool Operand::resolve(Process& owner, std::uint16_t& out) const {
    if (isLiteral) { out = literal; return true; }
    return owner.readVar(var, out);
}
