#include "Operand.h"
#include "Process.h"

std::uint16_t Operand::resolve(Process& owner) const {
    if (isLiteral) return literal;

    SymbolTable& tbl = owner.getSymbolTable();
    if (!tbl.hasVariable(var))
        tbl.setVariable(var, 0); // auto-declare missing variables as 0 per spec

    int v = tbl.getVariable(var);
    // Clamp to uint16 range [0, 65535]
    if (v < 0)     v = 0;
    if (v > 65535) v = 65535;
    return static_cast<std::uint16_t>(v);
}
