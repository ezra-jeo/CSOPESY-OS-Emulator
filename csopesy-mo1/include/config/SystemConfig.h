#pragma once
#include <string>
#include <cstdint>

// Runtime configuration read from config.txt at `initialize` time (MO1 spec).
// This replaces the compile-time constants in Config.h for the MO1 build: the seed's
// Config.h is kept only so the original FCFS demo path still compiles.
//
// config.txt is space-separated `key value` lines, e.g.:
//   num-cpu 4
//   scheduler rr
//   quantum-cycles 5
//   batch-process-freq 1
//   min-ins 1000
//   max-ins 2000
//   delays-per-exec 0
//   max-overall-mem 16384
//   mem-per-frame 16
//   mem-per-proc 4096
struct SystemConfig {
    enum class Scheduler { FCFS, RR };

    // Defaults are placeholders; load() overwrites them from config.txt.
    std::int32_t  numCpu          = 4;       // [1, 128]
    Scheduler     scheduler       = Scheduler::FCFS;
    std::uint32_t quantumCycles   = 1;       // [1, 2^32-1]; RR only
    std::uint32_t batchProcessFreq = 1;      // [1, 2^32-1]
    std::uint32_t minIns          = 1;       // [1, 2^32-1]
    std::uint32_t maxIns          = 1;       // [1, 2^32-1]; must be >= minIns
    std::uint32_t delaysPerExec   = 0;       // [0, 2^32-1]; 0 = one instruction per tick

    // First-fit flat memory allocator (lecture: "Emulating a first-fit flat memory model").
    std::uint64_t maxOverallMem   = 16384;   // total main memory in bytes; >= memPerProc
    std::uint64_t memPerFrame     = 16;      // bytes per frame; >= 1
    std::uint64_t memPerProc      = 4096;    // fixed per-process footprint; >= 1

    // Parses `path`, validating every parameter against its allowed range.
    // Returns true on success; on failure returns false and fills `err` with a
    // human-readable reason (covers the "config must fall in valid values" requirement).
    bool load(const std::string& path, std::string& err);

    // Range/consistency check on the currently-held values (used by load()).
    bool validate(std::string& err) const;
};
