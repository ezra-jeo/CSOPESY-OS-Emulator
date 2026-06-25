#pragma once
#include "Process.h"
#include "SystemConfig.h"
#include <memory>
#include <string>

// Builds processes on demand for `scheduler-start`. For the MO1 "increasing x/y/z" test
// case every process is seeded with x = y = z = 0 and the fixed instruction set
// (ADD/PRINT for x, y, z repeated 100 times — the unrolled FOR). See buildInstructions().
class ProcessGenerator {
public:
    explicit ProcessGenerator(const SystemConfig& cfg);

    // Creates the next process with an auto-generated name (e.g. "p01", "p02", ...).
    std::shared_ptr<Process> generate();

    // Creates a process with a caller-supplied name (used by `screen -s <name>`).
    std::shared_ptr<Process> generate(const std::string& name);

private:
    void buildInstructions(Process& proc);

    const SystemConfig& cfg;
    int nextPid = 1;
};
