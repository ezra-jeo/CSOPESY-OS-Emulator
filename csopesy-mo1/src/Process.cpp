#include "Process.h"
#include <ctime>
#include <sstream>

Process::Process(int pid, std::string name)
    : pid(pid), name(std::move(name)), currentState(READY), commandCounter(0) {
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

std::time_t Process::getStartTime()  const { return startTime; }
std::time_t Process::getFinishTime() const { return finishTime; }

void Process::requestSleep(std::uint8_t ticks) { sleepPending = true; sleepTicks = ticks; }
bool Process::hasSleepRequest() const          { return sleepPending; }
std::uint8_t Process::getSleepTicks()   const  { return sleepTicks; }
void Process::clearSleepRequest()              { sleepPending = false; sleepTicks = 0; }

void Process::log(const std::string& line) {
    std::lock_guard<std::mutex> lk(logMutex);
    logs.push_back(line);
}

std::vector<std::string> Process::getLogs() const {
    std::lock_guard<std::mutex> lk(logMutex);
    return logs;
}

void Process::logMessage(const std::string& msg) {
    // std::localtime is not thread-safe in general, but each process's commands execute on
    // exactly one worker at a time, so this is safe here.
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "(%m/%d/%Y %I:%M:%S%p)", std::localtime(&t));
    std::ostringstream oss;
    oss << buf << " Core:" << coreId << " \"" << msg << "\"";
    log(oss.str());
}
