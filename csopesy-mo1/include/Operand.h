#pragma once
#include <string>
#include <cstdint>

class Process;

// An ADD/SUBTRACT operand is either a literal uint16 value or a variable name.
// Per spec, a referenced variable that does not yet exist is auto-declared as 0.
struct Operand {
    bool          isLiteral = true;
    std::uint16_t literal   = 0;
    std::string   var;

    static Operand fromLiteral(std::uint16_t v) { return {true, v, {}}; }
    static Operand fromVar(std::string name)    { return {false, 0, std::move(name)}; }

    // TODO(student): implement in a .cpp if you prefer — resolves this operand to a
    // concrete value, auto-declaring missing variables as 0 via owner's SymbolTable.
    std::uint16_t resolve(Process& owner) const;
};
