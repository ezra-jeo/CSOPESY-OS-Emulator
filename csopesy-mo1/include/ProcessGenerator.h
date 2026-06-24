#pragma once
#include "Process.h"
#include "SystemConfig.h"
#include <memory>
#include <string>

// Builds dummy processes on demand for `scheduler-start`. Each process gets a random
// instruction count in [min-ins, max-ins] and a randomized mix of PRINT commands.
// (Full ADD/DECLARE/SLEEP/FOR instruction mix is a separate TODO per MO1 spec.)
//
// Replaces the hardcoded preload loop in the seed's main.cpp.
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
