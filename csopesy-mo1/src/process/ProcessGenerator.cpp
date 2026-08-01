#include "ProcessGenerator.h"
#include "PrintCommand.h"
#include "DeclareCommand.h"
#include "AddCommand.h"
#include "SubtractCommand.h"
#include "SleepCommand.h"
#include "ForCommand.h"
#include "ReadCommand.h"
#include "WriteCommand.h"
#include <random>
#include <iomanip>
#include <sstream>
#include <cctype>
#include <algorithm>

namespace {

// Variable pool shared across all instruction types.
static const std::string VARS[] = { "x", "y", "z", "a", "b" };
static const int NVAR = 5;

// Single shared engine for the whole translation unit (generate(), buildInstructions(),
// makeFlat(), rollMemSize() all draw from this one instance).
static std::mt19937 rng(std::random_device{}());

// Rolls a uniformly-random power-of-two size in [cfg.minMemPerProc, cfg.maxMemPerProc].
// Both bounds are already guaranteed valid powers of two by SystemConfig::validate().
std::uint64_t rollMemSize(const SystemConfig& cfg, std::mt19937& rng) {
    std::vector<std::uint64_t> candidates;
    for (std::uint64_t v = cfg.minMemPerProc; v <= cfg.maxMemPerProc; v *= 2)
        candidates.push_back(v);
    std::uniform_int_distribution<std::size_t> dist(0, candidates.size() - 1);
    return candidates[dist(rng)];
}

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
// `memSize` is the OWNING process's address space size (unchanged across recursion levels — it
// describes the process, not the recursion depth) and bounds READ/WRITE's address operand to
// [0, memSize-2] so randomly-generated batch/screen-s processes exercise real page faults without
// ever hitting a spurious access violation (violations stay a deliberate, screen -c-only scenario).
std::vector<std::shared_ptr<ICommand>> makeFlat(
    int pid, const std::string& name, std::mt19937& rng, int depth, std::uint64_t memSize)
{
    const int maxType = (depth < 3) ? 8 : 7; // 0=PRINT 1=DECLARE 2=ADD 3=SUB 4=SLEEP 5=READ 6=WRITE 7=FOR
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
    case 5: { // READ
        if (memSize < 2) return { std::make_shared<DeclareCommand>(pid, VARS[vi(rng)], 0) };
        std::uniform_int_distribution<int> addrDist(0, static_cast<int>(memSize) - 2);
        std::uint16_t addr = static_cast<std::uint16_t>(addrDist(rng));
        return { std::make_shared<ReadCommand>(pid, VARS[vi(rng)], addr) };
    }
    case 6: { // WRITE
        if (memSize < 2) return { std::make_shared<DeclareCommand>(pid, VARS[vi(rng)], 0) };
        std::uniform_int_distribution<int> addrDist(0, static_cast<int>(memSize) - 2);
        std::uint16_t addr = static_cast<std::uint16_t>(addrDist(rng));
        return { std::make_shared<WriteCommand>(pid, addr, randOperand(rng)) };
    }
    case 7: { // FOR — build the body, construct a ForCommand, then flatten with iteration tags
        std::uniform_int_distribution<int> reps(1, 5);
        std::uniform_int_distribution<int> bodyLen(1, 3);
        const int r  = reps(rng);
        const int bl = bodyLen(rng);

        std::vector<std::shared_ptr<ICommand>> body;
        for (int i = 0; i < bl; ++i) {
            auto sub = makeFlat(pid, name, rng, depth + 1, memSize);
            body.insert(body.end(), sub.begin(), sub.end());
        }
        // ForCommand encapsulates the loop structure; flatten() emits body×r annotated leaves
        // (e.g. "ADD(x,y,1)  [FOR i=2/3]") without storing the ForCommand in the commandList.
        return ForCommand(pid, std::move(body), r).flatten();
    }
    default:
        return { std::make_shared<PrintCommand>(pid, "Hello world from " + name + "!") };
    }
}

// ── screen -c instruction-text parser ────────────────────────────────────────

std::string trim(const std::string& s) {
    std::size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    std::size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

// Splits `text` on ';', but never on a ';' inside a "..." span. Trims each piece and drops a
// trailing empty piece (from a trailing ';').
std::vector<std::string> splitInstructions(const std::string& text) {
    std::vector<std::string> out;
    std::string cur;
    bool inQuotes = false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '"' && (i == 0 || text[i - 1] != '\\')) inQuotes = !inQuotes;
        if (c == ';' && !inQuotes) {
            out.push_back(trim(cur));
            cur.clear();
        } else {
            cur += c;
        }
    }
    std::string last = trim(cur);
    if (!last.empty()) out.push_back(last);
    return out;
}

// Splits `rest` on whitespace runs into tokens.
std::vector<std::string> splitTokens(const std::string& rest) {
    std::vector<std::string> out;
    std::istringstream iss(rest);
    for (std::string tok; iss >> tok; ) out.push_back(tok);
    return out;
}

// Splits `piece` into (opcode, rest) on the first whitespace run, OR on '(' if that comes first
// with no separating space (PRINT's own examples appear both as "PRINT(...)" and "PRINT( ...)").
bool splitOpcode(const std::string& piece, std::string& opcode, std::string& rest) {
    std::size_t i = 0;
    while (i < piece.size() && !std::isspace(static_cast<unsigned char>(piece[i])) && piece[i] != '(') ++i;
    if (i == 0) return false;
    opcode = piece.substr(0, i);
    while (i < piece.size() && std::isspace(static_cast<unsigned char>(piece[i]))) ++i;
    rest = piece.substr(i);
    return true;
}

// A token parses as a non-negative decimal integer fitting uint16_t.
bool tryParseU16Decimal(const std::string& token, std::uint16_t& out) {
    if (token.empty()) return false;
    for (char c : token) if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    try {
        unsigned long v = std::stoul(token);
        if (v > 65535UL) return false;
        out = static_cast<std::uint16_t>(v);
        return true;
    } catch (...) {
        return false;
    }
}

Operand parseOperand(const std::string& token) {
    std::uint16_t v;
    if (tryParseU16Decimal(token, v)) return Operand::fromLiteral(v);
    return Operand::fromVar(token);
}

// Token must begin with 0x/0X; the remainder parses as base-16 and fits [0, 65535].
bool parseHexAddr(const std::string& token, std::uint16_t& out) {
    if (token.size() < 3) return false;
    if (token[0] != '0' || (token[1] != 'x' && token[1] != 'X')) return false;
    std::string digits = token.substr(2);
    if (digits.empty()) return false;
    for (char c : digits) if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
    try {
        unsigned long v = std::stoul(digits, nullptr, 16);
        if (v > 65535UL) return false;
        out = static_cast<std::uint16_t>(v);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace

ProcessGenerator::ProcessGenerator(const SystemConfig& cfg) : cfg(cfg) {}

std::shared_ptr<Process> ProcessGenerator::generate() {
    const int pid = nextPid++;
    std::ostringstream oss;
    oss << "p" << std::setw(2) << std::setfill('0') << pid;
    auto proc = std::make_shared<Process>(pid, oss.str());
    std::uint64_t size = rollMemSize(cfg, rng);
    proc->setRequestedMemSize(size);
    buildInstructions(*proc, size);
    return proc;
}

std::shared_ptr<Process> ProcessGenerator::generate(const std::string& name) {
    const int pid = nextPid++;
    auto proc = std::make_shared<Process>(pid, name);
    std::uint64_t size = rollMemSize(cfg, rng);
    proc->setRequestedMemSize(size);
    buildInstructions(*proc, size);
    return proc;
}

std::shared_ptr<Process> ProcessGenerator::createEmpty(const std::string& name) {
    const int pid = nextPid++;
    return std::make_shared<Process>(pid, name);
}

bool ProcessGenerator::buildFromInstructionText(Process& proc, const std::string& text, std::string& err) {
    auto pieces = splitInstructions(text);
    if (pieces.empty() || pieces.size() > 50) {
        err = "invalid command";
        return false;
    }

    const int pid = proc.getPID();

    for (const auto& piece : pieces) {
        std::string opcode, rest;
        if (!splitOpcode(piece, opcode, rest)) { err = "invalid command"; return false; }

        if (opcode == "DECLARE") {
            auto toks = splitTokens(rest);
            std::uint16_t value;
            if (toks.size() != 2 || !tryParseU16Decimal(toks[1], value)) {
                err = "invalid command";
                return false;
            }
            proc.addCommand(std::make_shared<DeclareCommand>(pid, toks[0], value));
        } else if (opcode == "ADD" || opcode == "SUBTRACT") {
            auto toks = splitTokens(rest);
            if (toks.size() != 3) { err = "invalid command"; return false; }
            Operand op1 = parseOperand(toks[1]);
            Operand op2 = parseOperand(toks[2]);
            if (opcode == "ADD")
                proc.addCommand(std::make_shared<AddCommand>(pid, toks[0], op1, op2));
            else
                proc.addCommand(std::make_shared<SubtractCommand>(pid, toks[0], op1, op2));
        } else if (opcode == "WRITE") {
            auto toks = splitTokens(rest);
            std::uint16_t addr;
            if (toks.size() != 2 || !parseHexAddr(toks[0], addr)) { err = "invalid command"; return false; }
            Operand value = parseOperand(toks[1]);
            proc.addCommand(std::make_shared<WriteCommand>(pid, addr, value));
        } else if (opcode == "READ") {
            auto toks = splitTokens(rest);
            std::uint16_t addr;
            if (toks.size() != 2 || !parseHexAddr(toks[1], addr)) { err = "invalid command"; return false; }
            proc.addCommand(std::make_shared<ReadCommand>(pid, toks[0], addr));
        } else if (opcode == "PRINT") {
            std::string body = trim(rest);
            if (body.size() < 2 || body.front() != '(' || body.back() != ')') {
                err = "invalid command";
                return false;
            }
            std::string inner = trim(body.substr(1, body.size() - 2));
            if (inner.empty() || inner.front() != '"') { err = "invalid command"; return false; }

            std::size_t closeQuote = inner.find('"', 1);
            if (closeQuote == std::string::npos) { err = "invalid command"; return false; }
            std::string literal = inner.substr(1, closeQuote - 1);
            std::string after = trim(inner.substr(closeQuote + 1));

            if (after.empty()) {
                proc.addCommand(std::make_shared<PrintCommand>(pid, literal));
            } else if (after.front() == '+') {
                std::string varName = trim(after.substr(1));
                if (varName.empty()) { err = "invalid command"; return false; }
                proc.addCommand(std::make_shared<PrintCommand>(pid, literal, varName));
            } else {
                err = "invalid command";
                return false;
            }
        } else {
            err = "invalid command";
            return false;
        }
    }

    return true;
}

void ProcessGenerator::buildInstructions(Process& proc, std::uint64_t memSize) {
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
        auto flat = makeFlat(pid, name, rng, 0, memSize);
        for (auto& c : flat) {
            if (ctr >= count) break;
            proc.addCommand(c);
            ++ctr;
        }
    }
}
