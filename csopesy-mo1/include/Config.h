#pragma once

// All compile-time constants for Phase 1.
namespace Config {
    constexpr int  NUM_CORES           = 4;
    constexpr int  NUM_PROCESSES       = 10;
    constexpr int  PRINTS_PER_PROCESS  = 100;
    constexpr int  EXEC_DELAY_MS       = 100;  // per-instruction delay makes the run observable
constexpr const char* NAME_PREFIX  = "process_";
}
