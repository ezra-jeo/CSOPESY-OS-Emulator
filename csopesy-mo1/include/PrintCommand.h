#pragma once
#include "ICommand.h"
#include <string>

// Concrete command: writes a timestamped line to the owning process's log file.
class PrintCommand : public ICommand {
public:
    // Plain message form: PRINT("...").
    PrintCommand(int pid, const std::string& toPrint);

    // Interpolated form: PRINT("prefix" + var) — appends the *current* value of `varName`
    // (resolved from the symbol table at execution time) to `prefix`.
    PrintCommand(int pid, std::string prefix, std::string varName);

    void execute(Process& owner) override;

private:
    std::string toPrint; // prefix (or full message when hasVar == false)
    std::string varName;  // variable to append when hasVar == true
    bool        hasVar = false;
};
