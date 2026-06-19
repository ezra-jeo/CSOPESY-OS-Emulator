#pragma once

// All compile-time constants for Phase 1.
// Do NOT read from config.txt this phase — values live here only.
namespace Config {
    constexpr int  NUM_CORES           = 4;
    constexpr int  NUM_PROCESSES       = 10;
    constexpr int  PRINTS_PER_PROCESS  = 100;
    constexpr int  EXEC_DELAY_MS       = 20;   // per-instruction delay makes the run observable
    constexpr bool ENABLE_FILE_LOGGING = true;
    constexpr const char* OUTPUT_DIR   = "output";
    constexpr const char* NAME_PREFIX  = "process_";
}
