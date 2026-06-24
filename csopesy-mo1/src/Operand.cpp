#include "Operand.h"
#include "Process.h"

std::uint16_t Operand::resolve(Process& owner) const {
    // TODO(student): if isLiteral, return literal; otherwise read `var` from
    // owner.getSymbolTable() (auto-declaring to 0 when absent) and return it.
    (void)owner;
    return isLiteral ? literal : 0;
}
