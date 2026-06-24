#include "ProcessGenerator.h"
#include "PrintCommand.h"
#include <random>
#include <iomanip>
#include <sstream>

ProcessGenerator::ProcessGenerator(const SystemConfig& cfg) : cfg(cfg) {}

std::shared_ptr<Process> ProcessGenerator::generate() {
    const int pid = nextPid++;
    std::ostringstream oss;
    oss << "p" << std::setw(2) << std::setfill('0') << pid;
    auto proc = std::make_shared<Process>(pid, oss.str());
    buildInstructions(*proc);
    return proc;
}

std::shared_ptr<Process> ProcessGenerator::generate(const std::string& name) {
    const int pid = nextPid++;
    auto proc = std::make_shared<Process>(pid, name);
    buildInstructions(*proc);
    return proc;
}

void ProcessGenerator::buildInstructions(Process& proc) {
    // Pick a random instruction count in [minIns, maxIns].
    // Full instruction-type mix (DECLARE/ADD/SUBTRACT/SLEEP/FOR) is a separate TODO.
    static std::mt19937 rng(std::random_device{}());

    const std::uint32_t lo = cfg.minIns;
    const std::uint32_t hi = (cfg.maxIns >= lo) ? cfg.maxIns : lo;
    std::uniform_int_distribution<std::uint32_t> dist(lo, hi);
    const std::uint32_t count = dist(rng);

    const std::string msg = "Hello world from " + proc.getName() + "!";
    for (std::uint32_t i = 0; i < count; ++i)
        proc.addCommand(std::make_shared<PrintCommand>(proc.getPID(), msg));
}
