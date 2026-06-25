#include "ProcessGenerator.h"
#include "PrintCommand.h"
#include "AddCommand.h"
#include <iomanip>
#include <sstream>

ProcessGenerator::ProcessGenerator(const SystemConfig& cfg) : cfg(cfg) {}

std::shared_ptr<Process> ProcessGenerator::generate() {
    const int pid = nextPid++;
    std::ostringstream oss;
    oss << "p" << std::setw(2) << std::setfill('0') << pid;
    auto proc = std::make_shared<Process>(pid, oss.str());
    buildInstructions(*proc);
    return proc;
}

std::shared_ptr<Process> ProcessGenerator::generate(const std::string& name) {
    const int pid = nextPid++;
    auto proc = std::make_shared<Process>(pid, name);
    buildInstructions(*proc);
    return proc;
}

// Fixed instruction set for the MO1 "increasing x/y/z" test case. Every process starts
// with x = y = z = 0 and runs the equivalent of
//   FOR([ADD(x,x,1), PRINT("Value from: "+x),
//        ADD(y,y,1), PRINT("Value from: "+y),
//        ADD(z,z,1), PRINT("Value from: "+z)], 100)
// flattened into 600 top-level commands so RR (quantum-cycles) preempts between them and
// process-smi's instruction-line counter advances. min-ins/max-ins are intentionally
// ignored here — the set is fixed.
void ProcessGenerator::buildInstructions(Process& proc) {
    const int pid = proc.getPID();

    proc.getSymbolTable().setVariable("x", 0);
    proc.getSymbolTable().setVariable("y", 0);
    proc.getSymbolTable().setVariable("z", 0);

    for (int i = 0; i < 100; ++i) {
        proc.addCommand(std::make_shared<AddCommand>(pid, "x", Operand::fromVar("x"), Operand::fromLiteral(1)));
        proc.addCommand(std::make_shared<PrintCommand>(pid, "Value from: ", std::string("x")));
        proc.addCommand(std::make_shared<AddCommand>(pid, "y", Operand::fromVar("y"), Operand::fromLiteral(1)));
        proc.addCommand(std::make_shared<PrintCommand>(pid, "Value from: ", std::string("y")));
        proc.addCommand(std::make_shared<AddCommand>(pid, "z", Operand::fromVar("z"), Operand::fromLiteral(1)));
        proc.addCommand(std::make_shared<PrintCommand>(pid, "Value from: ", std::string("z")));
    }
}
