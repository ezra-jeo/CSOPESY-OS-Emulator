#include "ProcessGenerator.h"
#include "PrintCommand.h"

ProcessGenerator::ProcessGenerator(const SystemConfig& cfg) : cfg(cfg) {}

std::shared_ptr<Process> ProcessGenerator::generate() {
    // TODO(student): create a Process with a unique pid and a human-readable name
    //   (e.g. "process_07"), call buildInstructions() to fill it, and return it.
    const int pid = nextPid++;
    auto proc = std::make_shared<Process>(pid, "process_" + std::to_string(pid));
    buildInstructions(*proc);
    return proc;
}

void ProcessGenerator::buildInstructions(Process& proc) {
    // TODO(student): pick a random instruction count in [cfg.minIns, cfg.maxIns] and add
    //   a randomized mix of PRINT/DECLARE/ADD/SUBTRACT/SLEEP/FOR commands. Enforce the
    //   FOR nesting cap of 3. Spec PRINT default message: "Hello world from <name>!".
    //
    //   Stub: add a single PRINT so generated processes are non-empty and the base runs.
    proc.addCommand(std::make_shared<PrintCommand>(
        proc.getPID(), "Hello world from " + proc.getName() + "!"));
}
