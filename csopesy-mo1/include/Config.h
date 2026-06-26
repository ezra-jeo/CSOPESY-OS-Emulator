#pragma once

// All compile-time constants for Phase 1.
namespace Config {
    constexpr int  NUM_CORES           = 4;
    constexpr int  NUM_PROCESSES       = 10;
    constexpr int  PRINTS_PER_PROCESS  = 100;
    constexpr int  EXEC_DELAY_MS       = 100;  // per-instruction delay makes the run observable
    constexpr const char* NAME_PREFIX  = "process_";

    // When true, every executed command (ADD/SUBTRACT/DECLARE/SLEEP) appends a descriptive
    // line to the process log for monitoring (e.g. ADD(x, x, 4) => x = 8), visible in
    // process-smi. PRINT always logs its own output regardless. Set false to restore the
    // spec's PRINT-only process-smi view.
    constexpr bool LOG_PER_COMMAND     = true;
}
