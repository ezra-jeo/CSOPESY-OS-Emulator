#pragma once
#include "ICommand.h"
#include <string>
#include <cstdint>

// MO2 instruction: READ(var, memory_address)
// Reads a uint16 value from the process's own address space at `addr` and stores it into the
// named variable `var`. If the address was never written, the value is 0 (Process::memRead over
// a freshly-installed page returns zero-filled bytes). `addr` is a process-relative virtual
// address already parsed from its hex-literal source form by whoever builds this instruction.
class ReadCommand : public ICommand {
public:
    ReadCommand(int pid, std::string destVar, std::uint16_t addr);
    void execute(Process& owner) override;
    std::string toString() const override;

private:
    std::string   destVar;
    std::uint16_t addr;
};
