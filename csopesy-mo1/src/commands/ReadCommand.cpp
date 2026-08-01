#include "ReadCommand.h"
#include "Process.h"
#include <sstream>
#include <iomanip>

ReadCommand::ReadCommand(int pid, std::string destVar, std::uint16_t addr)
    : ICommand(pid, READ), destVar(std::move(destVar)), addr(addr) {}

void ReadCommand::execute(Process& owner) {
    std::uint16_t value;
    if (!owner.memRead(addr, value)) return;   // fault set; CPUWorker restarts
    owner.declareVar(destVar, value);
}

std::string ReadCommand::toString() const {
    std::ostringstream oss;
    oss << "READ(" << destVar << ", 0x" << std::hex << std::uppercase << addr << ")";
    return oss.str();
}
