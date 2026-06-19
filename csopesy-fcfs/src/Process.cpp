#include "Process.h"
#include "Config.h"
// TODO(student): uncomment the line below once you are ready to implement the constructor.
// #include <filesystem>

// ═══════════════════════════════════════════════════════════════════════════════
// ALL method bodies in this file are TODO stubs.
// The project compiles and the CLI runs, but processes never execute until you
// implement every method below.  Follow the numbered STEP comments exactly.
// ═══════════════════════════════════════════════════════════════════════════════

// ── Constructor ───────────────────────────────────────────────────────────────
// STEP 1: Initialise every member using the initialiser list:
//           pid(pid), name(std::move(name)), currentState(READY), commandCounter(0)
//         (coreId, startTime, finishTime are zero-initialised by their in-class
//          initialisers in Process.h — nothing extra needed for them.)
//
// STEP 2: If Config::ENABLE_FILE_LOGGING is true:
//   a. Create the output directory:
//        std::filesystem::create_directories(Config::OUTPUT_DIR);
//   b. Build the file path:
//        std::string path = std::string(Config::OUTPUT_DIR) + "/" + this->name + ".txt";
//   c. Open logFile:
//        logFile.open(path);
//   d. Write the header (exactly as shown, including the blank line):
//        logFile << "Process name: " << this->name << "\n";
//        logFile << "Logs:\n\n";
Process::Process(int pid, std::string name)
    : pid(pid), name(std::move(name)), currentState(READY), commandCounter(0) {
    // TODO(student): implement STEP 2 (file-logging setup) here.
}

// ── addCommand ────────────────────────────────────────────────────────────────
// STEP: commandList.push_back(command);
void Process::addCommand(std::shared_ptr<ICommand> command) {
    // TODO(student): implement
}

// ── executeCurrentCommand ─────────────────────────────────────────────────────
// STEP: if (!isFinished()) commandList[commandCounter]->execute(*this);
void Process::executeCurrentCommand() {
    // TODO(student): implement
}

// ── moveToNextLine ────────────────────────────────────────────────────────────
// STEP: commandCounter++;
void Process::moveToNextLine() {
    // TODO(student): implement
}

// ── isFinished ────────────────────────────────────────────────────────────────
// STEP: return commandCounter >= (int)commandList.size();
bool Process::isFinished() const {
    return false; // TODO(student): replace with real check
}

// ── getPID / getState / getName / getSymbolTable ──────────────────────────────
// STEP: return the corresponding private member.
int Process::getPID() const {
    return pid; // TODO(student): return pid
}

Process::ProcessState Process::getState() const {
    return currentState; // TODO(student): return currentState
}

std::string Process::getName() const {
    return name; // TODO(student): return name
}

SymbolTable& Process::getSymbolTable() {
    return symbolTable; // TODO(student): return symbolTable
}

// ── setState ──────────────────────────────────────────────────────────────────
// STEP 1: currentState = s;
// STEP 2: if (s == RUNNING)  startTime  = std::time(nullptr);
// STEP 3: if (s == FINISHED) finishTime = std::time(nullptr);
void Process::setState(ProcessState s) {
    // TODO(student): implement
}

// ── setCoreId / getCoreId ─────────────────────────────────────────────────────
// STEP (setCoreId): coreId = core;
void Process::setCoreId(int core) {
    // TODO(student): implement
}

int Process::getCoreId() const {
    return coreId; // TODO(student): return coreId
}

// ── getCommandCounter / getTotalCommands ──────────────────────────────────────
int Process::getCommandCounter() const {
    return commandCounter; // TODO(student): return commandCounter
}

int Process::getTotalCommands() const {
    return (int)commandList.size(); // TODO(student): return commandList.size()
}

// ── log ───────────────────────────────────────────────────────────────────────
// STEP: if (Config::ENABLE_FILE_LOGGING) logFile << line << '\n';
//
// NOTE: In non-preemptive FCFS, exactly one worker touches a given process
//       from start to finish without interruption, so no per-file mutex is needed.
void Process::log(const std::string& line) {
    // TODO(student): implement
}

// ── getStartTime / getFinishTime ──────────────────────────────────────────────
std::time_t Process::getStartTime() const {
    return startTime; // TODO(student): return startTime
}

std::time_t Process::getFinishTime() const {
    return finishTime; // TODO(student): return finishTime
}
