#pragma once
#include <unordered_map>
#include <string>
#include <cstddef>

// PCB component: stores named integer variables local to one process.
// Not exercised in Phase 1 (PRINT uses no variables); kept for forward compatibility.
//
// MO2 demand paging (Step 3) adds offset tracking alongside the original value-map API:
// variables live in the process's own paged memory (page 0, the "symbol table segment"),
// addressed by the 2-byte-aligned offsets assigned here. The old setVariable/getVariable/
// hasVariable map is left untouched — command files still using it (DeclareCommand, AddCommand,
// Operand, PrintCommand) are migrated to the offset-backed API in a later step.
class SymbolTable {
public:
    static constexpr std::size_t SEGMENT_SIZE = 64;  // bytes
    static constexpr std::size_t MAX_VARS     = 32;  // 32 * 2 bytes = 64

    void setVariable(const std::string& name, int value) { table[name] = value; }

    int getVariable(const std::string& name) {
        if (table.find(name) != table.end()) return table[name];
        return 0;
    }

    bool hasVariable(const std::string& name) const {
        return table.find(name) != table.end();
    }

    // Returns the 2-byte-aligned offset [0, 62] for `name` inside the 64-byte symbol table
    // segment, assigning a new offset if `name` is not yet known. Returns -1 if `name` is new
    // AND all 32 slots are already assigned (spec: "succeeding instructions involving variable
    // declarations will be ignored" — the caller must treat -1 as a silent no-op, not an error).
    int offsetFor(const std::string& name) {
        auto it = offsets.find(name);
        if (it != offsets.end()) return it->second;
        if (offsets.size() >= MAX_VARS) return -1;
        int assigned = nextOffset;
        nextOffset += 2;
        offsets[name] = assigned;
        return assigned;
    }

    // True if `name` already has an assigned offset (does not assign one).
    bool hasOffset(const std::string& name) const {
        return offsets.find(name) != offsets.end();
    }

private:
    std::unordered_map<std::string, int> table;
    std::unordered_map<std::string, int> offsets;
    int nextOffset = 0;
};
