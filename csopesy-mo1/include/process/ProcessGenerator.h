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

    // Allocates a fresh PID/name pair and returns a bare Process with NO instructions — the
    // caller (screen -c) populates it from user-supplied text instead of buildInstructions()'s
    // random mix.
    std::shared_ptr<Process> createEmpty(const std::string& name);

    // Parses `text` (semicolon-separated instructions, 1-50 of them) and appends the resulting
    // commands onto `proc`. Returns false and sets err = "invalid command" on ANY parse failure
    // (wrong instruction count, unknown opcode, wrong argument count, unparseable number/address)
    // — the whole batch is all-or-nothing; `proc` may be partially populated on failure, the
    // caller must discard it.
    bool buildFromInstructionText(Process& proc, const std::string& text, std::string& err);

private:
    void buildInstructions(Process& proc);

    const SystemConfig& cfg;
    int nextPid = 1;
};
