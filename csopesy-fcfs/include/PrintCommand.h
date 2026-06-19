#pragma once
#include "ICommand.h"
#include <string>

// Concrete command: writes a timestamped line to the owning process's log file.
class PrintCommand : public ICommand {
public:
    PrintCommand(int pid, const std::string& toPrint);
    void execute(Process& owner) override;

private:
    std::string toPrint; // e.g. "Hello world from process_01!"
};
