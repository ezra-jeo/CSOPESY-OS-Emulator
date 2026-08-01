#include "Process.h"
#include <algorithm>
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

std::vector<std::string> Process::getInstructionListing() const {
    std::vector<std::string> out;
    out.reserve(commandList.size());
    for (const auto& cmd : commandList)
        out.push_back(cmd->toString());
    return out;
}

void Process::setMemory(std::uint64_t base, std::uint64_t size) {
    baseAddress  = base;
    memSize      = size;
    memAllocated = true;
}

bool          Process::hasMemory()      const { return memAllocated; }
std::uint64_t Process::getBaseAddress() const { return baseAddress; }
std::uint64_t Process::getMemSize()     const { return memSize; }

void          Process::setRequestedMemSize(std::uint64_t size) { requestedMemSize = size; }
std::uint64_t Process::getRequestedMemSize() const { return requestedMemSize; }

// --- MO2 demand paging (Step 3) ---

void Process::bindMemory(std::uint64_t memSizeBytes, std::uint64_t pageSize, bool demandPaged) {
    memSize       = memSizeBytes;
    pageSizeBytes = pageSize;
    paged         = demandPaged;

    memoryBytes.assign(memSizeBytes, 0);

    std::uint64_t pageCount = (pageSize == 0) ? 0 : (memSizeBytes + pageSize - 1) / pageSize;
    pageResident.assign(pageCount, !demandPaged);
}

bool          Process::isPaged()         const { return paged; }
std::uint64_t Process::getPageCount()    const { return pageResident.size(); }
std::uint64_t Process::getPageSizeBytes() const { return pageSizeBytes; }

Process::MemFault Process::getFault()     const { return fault; }
std::uint64_t      Process::getFaultPage() const { return faultPage; }

void Process::clearFault() {
    fault     = MemFault::None;
    faultPage = 0;
}

bool          Process::hasViolation()     const { return terminatedByViolation; }
std::time_t   Process::getViolationTime() const { return violationTime; }
std::uint16_t Process::getViolationAddr() const { return violationAddr; }

bool Process::memRead(std::uint16_t addr, std::uint16_t& out) {
    if (static_cast<std::uint64_t>(addr) + 2 > memSize) {
        fault     = MemFault::Violation;
        faultPage = 0;
        if (!terminatedByViolation) {
            terminatedByViolation = true;
            violationTime = std::time(nullptr);
            violationAddr = addr;
        }
        return false;
    }

    std::uint64_t page = addr / pageSizeBytes;
    if (paged && !pageResident[page]) {
        fault     = MemFault::PageFault;
        faultPage = page;
        return false;
    }

    // Shared contract: memory values are stored little-endian across memRead/memWrite/
    // extractPageBytes/installPageBytes — other steps' paging code relies on this ordering.
    out = static_cast<std::uint16_t>(memoryBytes[addr]) |
          (static_cast<std::uint16_t>(memoryBytes[addr + 1]) << 8);
    return true;
}

bool Process::memWrite(std::uint16_t addr, std::uint16_t value) {
    if (static_cast<std::uint64_t>(addr) + 2 > memSize) {
        fault     = MemFault::Violation;
        faultPage = 0;
        if (!terminatedByViolation) {
            terminatedByViolation = true;
            violationTime = std::time(nullptr);
            violationAddr = addr;
        }
        return false;
    }

    std::uint64_t page = addr / pageSizeBytes;
    if (paged && !pageResident[page]) {
        fault     = MemFault::PageFault;
        faultPage = page;
        return false;
    }

    memoryBytes[addr]     = static_cast<std::uint8_t>(value & 0xFF);
    memoryBytes[addr + 1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
    return true;
}

bool Process::declareVar(const std::string& name, std::uint16_t value) {
    int offset = symbolTable.offsetFor(name);
    if (offset < 0) {
        // Table full and name is new: silent no-op per spec, not an error.
        return true;
    }
    return memWrite(static_cast<std::uint16_t>(offset), value);
}

bool Process::readVar(const std::string& name, std::uint16_t& out) {
    if (!symbolTable.hasOffset(name)) {
        if (!declareVar(name, 0)) return false;
    }
    int offset = symbolTable.offsetFor(name);
    if (offset < 0) {
        out = 0;
        return true;
    }
    return memRead(static_cast<std::uint16_t>(offset), out);
}

bool Process::writeVar(const std::string& name, std::uint16_t value) {
    return declareVar(name, value);
}

bool Process::isPageResident(std::uint64_t page) const {
    if (page >= pageResident.size()) return false;
    return pageResident[page];
}

std::vector<std::uint8_t> Process::extractPageBytes(std::uint64_t page) const {
    std::vector<std::uint8_t> out;
    if (page >= pageResident.size()) return out;

    std::uint64_t start = page * pageSizeBytes;
    std::uint64_t end   = std::min(start + pageSizeBytes, memSize);
    out.assign(memoryBytes.begin() + start, memoryBytes.begin() + end);
    return out;
}

void Process::installPageBytes(std::uint64_t page, const std::vector<std::uint8_t>& bytes) {
    if (page >= pageResident.size()) return;

    std::uint64_t start = page * pageSizeBytes;
    std::uint64_t end   = std::min(start + pageSizeBytes, memSize);
    std::uint64_t count = std::min(static_cast<std::uint64_t>(bytes.size()), end - start);
    std::copy(bytes.begin(), bytes.begin() + count, memoryBytes.begin() + start);
    pageResident[page] = true;
}

void Process::invalidatePage(std::uint64_t page) {
    if (page >= pageResident.size()) return;
    pageResident[page] = false;
}
