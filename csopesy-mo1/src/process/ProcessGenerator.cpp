#include "ProcessGenerator.h"
#include "PrintCommand.h"
#include "DeclareCommand.h"
#include "AddCommand.h"
#include "SubtractCommand.h"
#include "SleepCommand.h"
#include "ForCommand.h"
#include <random>
#include <iomanip>
#include <sstream>

namespace {

// Variable pool shared across all instruction types.
static const std::string VARS[] = { "x", "y", "z", "a", "b" };
static const int NVAR = 5;

// Returns a random operand: 50% chance of literal [0, 65535], else a var from the pool.
Operand randOperand(std::mt19937& rng) {
    std::uniform_int_distribution<int> coin(0, 1);
    if (coin(rng)) {
        std::uniform_int_distribution<int> val(0, 65535);
        return Operand::fromLiteral(static_cast<std::uint16_t>(val(rng)));
    } else {
        std::uniform_int_distribution<int> vi(0, NVAR - 1);
        return Operand::fromVar(VARS[vi(rng)]);
    }
}

// Forward declaration so FOR can recurse.
std::shared_ptr<ICommand> makeRandomCommand(
    int pid, const std::string& name, std::mt19937& rng, int depth);

// Generates a flat sub-body of 1–3 commands for a FOR block.
std::vector<std::shared_ptr<ICommand>> makeBody(
    int pid, const std::string& name, std::mt19937& rng, int depth)
{
    std::uniform_int_distribution<int> bodyLen(1, 3);
    int len = bodyLen(rng);
    std::vector<std::shared_ptr<ICommand>> body;
    body.reserve(len);
    for (int i = 0; i < len; ++i)
        body.push_back(makeRandomCommand(pid, name, rng, depth));
    return body;
}

std::shared_ptr<ICommand> makeRandomCommand(
    int pid, const std::string& name, std::mt19937& rng, int depth)
{
    // Available types at current depth; FOR is excluded at depth >= 3 (spec cap).
    const int maxType = (depth < 3) ? 6 : 5; // 0=PRINT 1=DECLARE 2=ADD 3=SUB 4=SLEEP 5=FOR
    std::uniform_int_distribution<int> typeDist(0, maxType - 1);
    int type = typeDist(rng);

    std::uniform_int_distribution<int> vi(0, NVAR - 1);

    switch (type) {
    case 0: // PRINT
        return std::make_shared<PrintCommand>(pid, "Hello world from " + name + "!");
    case 1: { // DECLARE
        std::uniform_int_distribution<int> val(0, 65535);
        return std::make_shared<DeclareCommand>(
            pid, VARS[vi(rng)], static_cast<std::uint16_t>(val(rng)));
    }
    case 2: // ADD
        return std::make_shared<AddCommand>(pid, VARS[vi(rng)], randOperand(rng), randOperand(rng));
    case 3: // SUBTRACT
        return std::make_shared<SubtractCommand>(pid, VARS[vi(rng)], randOperand(rng), randOperand(rng));
    case 4: { // SLEEP
        std::uniform_int_distribution<int> ticks(1, 5); // small range to avoid long waits
        return std::make_shared<SleepCommand>(pid, static_cast<std::uint8_t>(ticks(rng)));
    }
    default: { // FOR (depth < 3)
        std::uniform_int_distribution<int> reps(1, 5);
        return std::make_shared<ForCommand>(
            pid, makeBody(pid, name, rng, depth + 1), reps(rng));
    }
    }
}

} // namespace

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
    static std::mt19937 rng(std::random_device{}());

    const std::uint32_t lo = cfg.minIns;
    const std::uint32_t hi = (cfg.maxIns >= lo) ? cfg.maxIns : lo;
    std::uniform_int_distribution<std::uint32_t> countDist(lo, hi);
    const std::uint32_t count = countDist(rng);

    const int pid = proc.getPID();
    const std::string& name = proc.getName();
    std::uint32_t ctr = 0;
    
    while (ctr < count) {
        auto cmd = makeRandomCommand(pid, name, rng, 0);
        // Get the instruction count and increase the ctr. 
        // Guard against exceeding, if using a for loop that exceeds the count, truncate for now.
        
        proc.addCommand(cmd);
    }
}
