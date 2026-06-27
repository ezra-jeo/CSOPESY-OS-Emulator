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

// Generates one random instruction as a FLAT list of leaf commands. A FOR is expanded here: its
// body is generated once and then repeated `reps` times, so the repeated commands act as a real
// loop (shared variables accumulate across iterations) and each iteration is a separate, counted,
// logged, preemptible instruction. Recurses for nested FORs; FOR is excluded at depth >= 3 (cap).
std::vector<std::shared_ptr<ICommand>> makeFlat(
    int pid, const std::string& name, std::mt19937& rng, int depth)
{
    const int maxType = (depth < 3) ? 6 : 5; // 0=PRINT 1=DECLARE 2=ADD 3=SUB 4=SLEEP 5=FOR
    std::uniform_int_distribution<int> typeDist(0, maxType - 1);
    std::uniform_int_distribution<int> vi(0, NVAR - 1);
    int type = typeDist(rng);

    switch (type) {
    case 0: // PRINT
        return { std::make_shared<PrintCommand>(pid, "Hello world from " + name + "!") };
    case 1: { // DECLARE
        std::uniform_int_distribution<int> val(0, 65535);
        return { std::make_shared<DeclareCommand>(
            pid, VARS[vi(rng)], static_cast<std::uint16_t>(val(rng))) };
    }
    case 2: // ADD
        return { std::make_shared<AddCommand>(pid, VARS[vi(rng)], randOperand(rng), randOperand(rng)) };
    case 3: // SUBTRACT
        return { std::make_shared<SubtractCommand>(pid, VARS[vi(rng)], randOperand(rng), randOperand(rng)) };
    case 4: { // SLEEP
        std::uniform_int_distribution<int> ticks(1, 5);
        return { std::make_shared<SleepCommand>(pid, static_cast<std::uint8_t>(ticks(rng))) };
    }
    default: { // FOR — build the body, construct a ForCommand, then flatten with iteration tags
        std::uniform_int_distribution<int> reps(1, 5);
        std::uniform_int_distribution<int> bodyLen(1, 3);
        const int r  = reps(rng);
        const int bl = bodyLen(rng);

        std::vector<std::shared_ptr<ICommand>> body;
        for (int i = 0; i < bl; ++i) {
            auto sub = makeFlat(pid, name, rng, depth + 1);
            body.insert(body.end(), sub.begin(), sub.end());
        }
        // ForCommand encapsulates the loop structure; flatten() emits body×r annotated leaves
        // (e.g. "ADD(x,y,1)  [FOR i=2/3]") without storing the ForCommand in the commandList.
        return ForCommand(pid, std::move(body), r).flatten();
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

    // Fill to exactly `count` instructions. FOR loops are flattened by makeFlat, so their
    // body x repetitions count toward the limit individually; a final loop may be truncated.
    std::uint32_t ctr = 0;
    while (ctr < count) {
        auto flat = makeFlat(pid, name, rng, 0);
        for (auto& c : flat) {
            if (ctr >= count) break;
            proc.addCommand(c);
            ++ctr;
        }
    }
}
