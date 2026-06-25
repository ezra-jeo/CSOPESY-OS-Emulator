#pragma once
#include <unordered_map>
#include <string>

// PCB component: stores named integer variables local to one process.
// Not exercised in Phase 1 (PRINT uses no variables); kept for forward compatibility.
class SymbolTable {
public:
    void setVariable(const std::string& name, int value) { table[name] = value; }

    int getVariable(const std::string& name) {
        if (table.find(name) != table.end()) return table[name];
        return 0;
    }

    bool hasVariable(const std::string& name) const {
        return table.find(name) != table.end();
    }

private:
    std::unordered_map<std::string, int> table;
};
