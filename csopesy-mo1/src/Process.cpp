#include "Process.h"
#include "Config.h"
#include <filesystem>
#include <fstream>

Process::Process(int pid, std::string name)
    : pid(pid), name(std::move(name)), currentState(READY), commandCounter(0) {
    if (Config::ENABLE_FILE_LOGGING) {
        std::filesystem::create_directories(Config::OUTPUT_DIR);
        logPath = std::string(Config::OUTPUT_DIR) + "/" + this->name + ".txt";
        // Write header then close immediately — keeps fd count at O(1).
        std::ofstream f(logPath, std::ios::trunc);
        f << "Process name: " << this->name << "\n";
        f << "Logs:\n\n";
    }
}

void Process::addCommand(std::shared_ptr<ICommand> command) {
    commandList.push_back(command);
}

void Process::executeCurrentCommand() {
    if (!isFinished())
        commandList[commandCounter]->execute(*this);
}

void Process::moveToNextLine() {
    commandCounter++;
}

bool Process::isFinished() const {
    return commandCounter >= (int)commandList.size();
}

int Process::getPID() const { return pid; }

Process::ProcessState Process::getState() const { return currentState; }

std::string Process::getName() const { return name; }

SymbolTable& Process::getSymbolTable() { return symbolTable; }

void Process::setState(ProcessState s) {
    currentState = s;
    if (s == RUNNING)   startTime  = std::time(nullptr);
    if (s == FINISHED)  finishTime = std::time(nullptr);
}

void Process::setCoreId(int core) { coreId = core; }

int Process::getCoreId()         const { return coreId; }
int Process::getCommandCounter() const { return commandCounter; }
int Process::getTotalCommands()  const { return (int)commandList.size(); }

void Process::log(const std::string& line) {
    // Open-append-close per call: keeps file-descriptor count at O(1) regardless
    // of how many processes are alive. One worker owns the process at a time so
    // no lock is needed.
    if (Config::ENABLE_FILE_LOGGING && !logPath.empty()) {
        std::ofstream f(logPath, std::ios::app);
        f << line << '\n';
    }
}

std::time_t Process::getStartTime()  const { return startTime; }
std::time_t Process::getFinishTime() const { return finishTime; }

void Process::requestSleep(std::uint8_t ticks) { sleepPending = true; sleepTicks = ticks; }
bool Process::hasSleepRequest() const          { return sleepPending; }
std::uint8_t Process::getSleepTicks()   const  { return sleepTicks; }
void Process::clearSleepRequest()              { sleepPending = false; sleepTicks = 0; }
