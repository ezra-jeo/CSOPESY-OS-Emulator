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

    std::uint16_t resolve(Process& owner) const;

    // Source form for logging: the variable name, or the literal value as text.
    std::string toString() const {
        return isLiteral ? std::to_string(literal) : var;
    }
};
