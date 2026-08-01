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

    // Resolves this operand's current value into `out`. Returns false (and leaves a fault set on
    // `owner`, per Process::getFault()) if resolving requires a page that isn't resident, or the
    // operand is a variable whose auto-declare-on-first-use write itself faults. Callers MUST stop
    // and return immediately on false — do not proceed to any further reads/writes/logging.
    bool resolve(Process& owner, std::uint16_t& out) const;

    // Source form for logging: the variable name, or the literal value as text.
    std::string toString() const {
        return isLiteral ? std::to_string(literal) : var;
    }
};
