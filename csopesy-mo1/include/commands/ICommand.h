#pragma once
#include <string>

class Process; // forward-declared so execute() can take the owning PCB

// Base class for all instructions a process can execute.
//
// DEVIATION from lecture stub: execute() receives a Process& instead of void so
// that commands can read coreId / process name and call log() without a global
// registry.  Document this in your lab report.
class ICommand {
public:
    // PRINT/IO exist in the seeded FCFS base. The remaining types are MO1 additions.
    enum CommandType { IO, PRINT, DECLARE, ADD, SUBTRACT, SLEEP, FOR };

    ICommand(int pid, CommandType commandType);
    CommandType getCommandType();

    virtual void execute(Process& owner) = 0;

    // Source-like text of this instruction for the process-smi listing, e.g.
    // "ADD(x, x, 4)" or "FOR([PRINT(\"hi\")], 3)". Static (no runtime values).
    virtual std::string toString() const = 0;

    virtual ~ICommand() = default;

protected:
    int         pid;
    CommandType commandType;
};
