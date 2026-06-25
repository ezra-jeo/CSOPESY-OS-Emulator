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
        {"delays-per-exec",    &delaysPerExec},
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
    return true;
}
