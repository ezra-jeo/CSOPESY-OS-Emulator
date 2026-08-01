#include "SystemConfig.h"
#include <fstream>
#include <sstream>
#include <unordered_map>

bool SystemConfig::load(const std::string& path, std::string& err) {
    const std::unordered_map<std::string, Scheduler> schedulerMap = {
        {"fcfs", Scheduler::FCFS},
        {"rr",   Scheduler::RR}
    };

    const std::unordered_map<std::string, uint32_t*> numericFields = {
        {"quantum-cycles",     &quantumCycles},
        {"batch-process-freq", &batchProcessFreq},
        {"min-ins",            &minIns},
        {"max-ins",            &maxIns},
        // The spec is inconsistent: its parameter table says "delays-per-exec" but its sample
        // config / quiz write "delay-per-exec". Accept both spellings.
        {"delays-per-exec",    &delaysPerExec},
        {"delay-per-exec",     &delaysPerExec},
    };

    // First-fit memory allocator parameters — byte counts, so 64-bit.
    const std::unordered_map<std::string, uint64_t*> numericFields64 = {
        {"max-overall-mem",   &maxOverallMem},
        {"mem-per-frame",     &memPerFrame},
        {"mem-per-proc",      &memPerProc},
        {"min-mem-per-proc",  &minMemPerProc},
        {"max-mem-per-proc",  &maxMemPerProc},
    };

    std::ifstream file(path);
    if (!file) {
        err = "Cannot open config file: " + path;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::istringstream ss(line);
        std::string key, value;
        if (!(ss >> key >> value)) continue;

        // The spec writes string values quoted (e.g. scheduler "rr"); strip a surrounding
        // pair of double-quotes so both quoted and unquoted forms parse.
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
            value = value.substr(1, value.size() - 2);

        try {
            if (key == "scheduler") {
                auto it = schedulerMap.find(value);
                if (it == schedulerMap.end()) {
                    err = "Unknown scheduler '" + value + "': expected fcfs or rr";
                    return false;
                }
                scheduler = it->second;
            } else if (key == "num-cpu") {
                numCpu = static_cast<std::int32_t>(std::stoi(value));
            } else if (auto it64 = numericFields64.find(key); it64 != numericFields64.end()) {
                *it64->second = static_cast<uint64_t>(std::stoull(value));
                if (key == "min-mem-per-proc" || key == "max-mem-per-proc")
                    sawMinMaxMemPerProc = true;
            } else {
                auto it = numericFields.find(key);
                if (it == numericFields.end()) {
                    err = "Unknown config key: " + key;
                    return false;
                }
                *it->second = static_cast<uint32_t>(std::stoul(value));
            }
        } catch (const std::exception&) {
            err = "Invalid value for '" + key + "': " + value;
            return false;
        }
    }

    return validate(err);
}

namespace {

bool isPowerOfTwo(std::uint64_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

// All memory-size parameters share the spec's [2^6, 2^16] power-of-2 constraint.
bool validateMemSize(const std::string& key, std::uint64_t value, std::string& err) {
    if (!isPowerOfTwo(value)) {
        err = key + " must be a power of 2, got " + std::to_string(value);
        return false;
    }
    if (value < 64 || value > 65536) {
        err = key + " must be in [64, 65536], got " + std::to_string(value);
        return false;
    }
    return true;
}

}  // namespace

bool SystemConfig::validate(std::string& err) const {
    if (numCpu < 1 || numCpu > 128) {
        err = "num-cpu must be in [1, 128], got " + std::to_string(numCpu);
        return false;
    }
    if (quantumCycles < 1) {
        err = "quantum-cycles must be >= 1";
        return false;
    }
    if (batchProcessFreq < 1) {
        err = "batch-process-freq must be >= 1";
        return false;
    }
    if (minIns < 1) {
        err = "min-ins must be >= 1";
        return false;
    }
    if (maxIns < 1) {
        err = "max-ins must be >= 1";
        return false;
    }
    if (maxIns < minIns) {
        err = "max-ins (" + std::to_string(maxIns) + ") must be >= min-ins (" + std::to_string(minIns) + ")";
        return false;
    }
    // delaysPerExec: [0, 2^32-1] is the full uint32_t range, no check needed

    // Spec: "All memory ranges are [2^6, 2^16] and the power of 2 format." Applies to every
    // memory-size parameter. min/max-mem-per-proc default to valid values (64, 4096) so a
    // legacy MO1-only config that never mentions them still validates cleanly.
    if (!validateMemSize("max-overall-mem", maxOverallMem, err)) return false;
    if (!validateMemSize("mem-per-frame", memPerFrame, err)) return false;
    if (!validateMemSize("mem-per-proc", memPerProc, err)) return false;
    if (!validateMemSize("min-mem-per-proc", minMemPerProc, err)) return false;
    if (!validateMemSize("max-mem-per-proc", maxMemPerProc, err)) return false;

    if (minMemPerProc > maxMemPerProc) {
        err = "min-mem-per-proc (" + std::to_string(minMemPerProc) + ") must be <= max-mem-per-proc ("
            + std::to_string(maxMemPerProc) + ")";
        return false;
    }
    if (memPerProc > maxOverallMem) {
        err = "mem-per-proc (" + std::to_string(memPerProc) + ") must be <= max-overall-mem ("
            + std::to_string(maxOverallMem) + ")";
        return false;
    }
    return true;
}
