#include "SystemConfig.h"

bool SystemConfig::load(const std::string& path, std::string& err) {
    // TODO(student): open `path`, parse each `key value` line, and assign into the
    //   fields below. Recognise: num-cpu, scheduler (fcfs|rr), quantum-cycles,
    //   batch-process-freq, min-ins, max-ins, delays-per-exec. Reject unknown keys and
    //   non-numeric values. Then call validate(err) and return its result.
    (void)path;
    err = "SystemConfig::load not implemented yet";
    return false;
}

bool SystemConfig::validate(std::string& err) const {
    // TODO(student): enforce ranges and return false with a clear message on the first
    //   violation. Required checks:
    //     numCpu            in [1, 128]
    //     quantumCycles     in [1, 2^32-1]
    //     batchProcessFreq  in [1, 2^32-1]
    //     minIns            in [1, 2^32-1]
    //     maxIns            in [1, 2^32-1] AND maxIns >= minIns
    //     delaysPerExec     in [0, 2^32-1]
    //   (scheduler is already constrained by the enum once parsed.)
    (void)err;
    return true;
}
