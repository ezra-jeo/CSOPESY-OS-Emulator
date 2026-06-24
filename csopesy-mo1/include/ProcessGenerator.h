#pragma once
#include "Process.h"
#include "SystemConfig.h"
#include <memory>
#include <string>

// Builds dummy processes on demand for `scheduler-start`. Each process gets a random
// instruction count in [min-ins, max-ins] and a randomized mix of the MO1 instruction
// types (PRINT/DECLARE/ADD/SUBTRACT/SLEEP/FOR), with FOR nesting capped at 3 levels.
//
// Replaces the hardcoded preload loop in the seed's main.cpp.
class ProcessGenerator {
public:
    explicit ProcessGenerator(const SystemConfig& cfg);

    // Creates the next process (unique pid + human-readable name) fully populated with
    // a randomized instruction list. Does NOT enqueue it — the caller admits it to the
    // scheduler.
    std::shared_ptr<Process> generate();

private:
    // Builds a randomized instruction list for one process, honoring the nesting cap.
    void buildInstructions(Process& proc);

    const SystemConfig& cfg;
    int nextPid = 1;
};
