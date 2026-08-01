#include "WriteCommand.h"
#include "Process.h"
#include <sstream>
#include <iomanip>

WriteCommand::WriteCommand(int pid, std::uint16_t addr, Operand value)
    : ICommand(pid, WRITE), addr(addr), value(std::move(value)) {}

void WriteCommand::execute(Process& owner) {
    std::uint16_t v;
    if (!value.resolve(owner, v)) return;   // resolve first — restart-safe even if `value` is a variable that itself faults
    owner.memWrite(addr, v);
}

std::string WriteCommand::toString() const {
    std::ostringstream oss;
    oss << "WRITE(0x" << std::hex << std::uppercase << addr << ", " << value.toString() << ")";
    return oss.str();
}
