#pragma once
#include "ICommand.h"
#include <string>

// Concrete command: appends a timestamped line to the owning process's PRINT log.
class PrintCommand : public ICommand {
public:
    // Fixed message, e.g. PRINT("Hello world from p01!").
    PrintCommand(int pid, const std::string& toPrint);
    // Interpolated form, e.g. PRINT("Value from: " + x): prints prefix + the current value of
    // varName, resolved at execution time (spec §"Barebones process instructions").
    PrintCommand(int pid, std::string prefix, std::string varName);
    void execute(Process& owner) override;
    std::string toString() const override;

private:
    std::string toPrint;        // fixed text, or the prefix when hasVar is true
    std::string varName;        // variable to interpolate (when hasVar)
    bool        hasVar = false;
};
