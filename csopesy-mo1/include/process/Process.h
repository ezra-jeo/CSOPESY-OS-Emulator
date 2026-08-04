#pragma once
#include "ICommand.h"
#include "SymbolTable.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <ctime>
#include <cstdint>

// PCB (Process Control Block) — all state the OS needs to manage one process.
// Declaration matches the lecture spec; see the DEVIATION note below for additions.
class Process {
public:
    enum ProcessState { READY, RUNNING, WAITING, FINISHED };

    Process(int pid, std::string name);

    // Core PCB operations (lecture §3)
    void addCommand(std::shared_ptr<ICommand> command);
    void executeCurrentCommand();   // runs commandList[commandCounter]->execute(*this)
    void moveToNextLine();          // commandCounter++
    bool isFinished() const;        // commandCounter >= commandList.size()

    int          getPID()          const;
    ProcessState getState()        const;
    std::string  getName()         const;
    SymbolTable& getSymbolTable();

    // --- Minimal additions for Phase 1 (document as deviations in your report) ---
    void setState(ProcessState s);
    void setCoreId(int core);       // worker sets this before executing instructions
    int  getCoreId()               const;
    int  getCommandCounter()       const;   // for "X / 100" progress in screen -ls
    int  getTotalCommands()        const;   // commandList.size()
    std::time_t getStartTime()     const;   // set when state transitions to RUNNING
    std::time_t getFinishTime()    const;   // set when state transitions to FINISHED

    // Sleep-request flag: SleepCommand sets it; CPUWorker reads+clears it to yield the core.
    void          requestSleep(std::uint8_t ticks);
    bool          hasSleepRequest() const;
    std::uint8_t  getSleepTicks()   const;
    void          clearSleepRequest();

    // PRINT output log. The worker thread appends via log() while the console thread reads via
    // getLogs(); both lock logMutex. getLogs() returns a copy so callers iterate safely.
    void                     log(const std::string& line);
    std::vector<std::string> getLogs() const;
    // Wraps msg with the spec line format — (timestamp) Core:<id> "<msg>" — then logs it.
    void                     logMessage(const std::string& msg);

    // Source-like text of every instruction (FOR rendered with its body), index i = line i+1.
    // Used by process-smi to show the program with a pointer at the current line.
    std::vector<std::string> getInstructionListing() const;

    // --- Memory manager bookkeeping ---
    // A process keeps its memory block from first dispatch until it finishes;
    // preemption at the end of a quantum does NOT release it.
    void          setMemory(std::uint64_t base, std::uint64_t size);
    bool          hasMemory()      const;
    std::uint64_t getBaseAddress() const;
    std::uint64_t getMemSize()     const;

    // Communicates a specific memory size to use the FIRST time this process is admitted
    // (screen -s/-c give an explicit size). 0 (the default) means "unset" — SchedulerBase::
    // acquireMemory falls back to a small hardcoded defensive floor in that case.
    void          setRequestedMemSize(std::uint64_t size);
    std::uint64_t getRequestedMemSize() const;

    // --- MO2 demand paging (Step 3): virtual address space + fault channel ---
    enum class MemFault { None, PageFault, Violation };

    // Sets up this process's virtual address space. Called once, right after the allocator
    // secures memory for the process (mirrors the existing setMemory() call site in
    // SchedulerBase::acquireMemory).
    //   memSize     – total address space size in bytes
    //   pageSize    – bytes per page. Under demand paging this is mem-per-frame; under the flat
    //                 allocator, pass memSize itself so the whole space is exactly one page.
    //   demandPaged – true: pages start NOT resident (memRead/memWrite PageFault until a later
    //                 step's allocator installs them). false: pages start resident immediately
    //                 (flat model — memRead/memWrite only ever produce Violation, never PageFault).
    void bindMemory(std::uint64_t memSize, std::uint64_t pageSize, bool demandPaged);
    bool isPaged() const;
    std::uint64_t getPageCount() const;
    std::uint64_t getPageSizeBytes() const;

    // Fault channel, checked by a later step's CPU worker after every executed instruction.
    MemFault      getFault() const;
    std::uint64_t getFaultPage() const;   // meaningful only when getFault() == PageFault
    void          clearFault();           // clears transient fault state ONLY — does not clear
                                           // hasViolation() below

    // Permanent violation record: set once when a Violation fault first occurs, survives
    // clearFault(), read later by screen -r to report "shut down due to memory access
    // violation... at <time>. <addr> invalid."
    bool          hasViolation() const;
    std::time_t   getViolationTime() const;
    std::uint16_t getViolationAddr() const;

    // Raw absolute-address memory ops (process-relative virtual addresses, [0, memSize)).
    // On failure: returns false and getFault() reports why (Violation = out of bounds;
    // PageFault = valid address but page not resident). On success: returns true, *out set
    // (memRead) or byte written (memWrite).
    bool memRead(std::uint16_t addr, std::uint16_t& out);
    bool memWrite(std::uint16_t addr, std::uint16_t value);

    // Symbol-table-backed named variables. Offsets come from SymbolTable::offsetFor/hasOffset;
    // the actual 2-byte value lives in this process's own memory (via memRead/memWrite at that
    // offset), NOT in a side map — so these can page-fault exactly like a raw memory op when the
    // symbol-table page (page 0) isn't resident.
    //   declareVar: creates-or-updates `name` = value. If `name` is new and the table already
    //     has 32 vars, this is a silent no-op that still returns true (spec: ignored, not an
    //     error) — it must NOT attempt a memWrite in that case. If `name` already exists (or a
    //     slot was just assigned), memWrite the value at its offset; on a PageFault/Violation
    //     from that memWrite, propagate by returning false (getFault() reports why).
    //   readVar: if `name` isn't known yet, auto-declare it as 0 first (matches this codebase's
    //     existing Operand::resolve semantic: "a referenced variable that does not yet exist is
    //     auto-declared as 0"), then memRead its offset into *out. Propagate any fault by
    //     returning false.
    //   writeVar: same offset resolution as declareVar, then memWrite. Propagate faults.
    bool declareVar(const std::string& name, std::uint16_t value);
    bool readVar(const std::string& name, std::uint16_t& out);
    bool writeVar(const std::string& name, std::uint16_t value);

    // Page-table access for a later step's paging allocator (flat model never calls these).
    bool                      isPageResident(std::uint64_t page) const;
    std::vector<std::uint8_t> extractPageBytes(std::uint64_t page) const;              // for paging OUT
    void                      installPageBytes(std::uint64_t page, const std::vector<std::uint8_t>& bytes); // paging IN; also marks resident
    void                      invalidatePage(std::uint64_t page);                       // marks evicted

private:
    int          pid;
    std::string  name;
    ProcessState currentState;
    int          commandCounter;
    std::vector<std::shared_ptr<ICommand>> commandList;
    SymbolTable  symbolTable;
    int          coreId = -1;
    bool         sleepPending = false;
    std::uint8_t sleepTicks   = 0;

    std::time_t  startTime  = 0;
    std::time_t   finishTime = 0;

    std::vector<std::string> logs;
    mutable std::mutex       logMutex;

    bool          memAllocated = false;
    std::uint64_t baseAddress  = 0;
    std::uint64_t memSize      = 0;
    std::uint64_t requestedMemSize = 0;

    // --- MO2 demand paging (Step 3) ---
    bool                   paged        = false;
    std::uint64_t          pageSizeBytes = 0;
    std::vector<std::uint8_t> memoryBytes;
    std::vector<bool>         pageResident;

    MemFault      fault      = MemFault::None;
    std::uint64_t faultPage  = 0;

    bool          terminatedByViolation = false;
    std::time_t   violationTime         = 0;
    std::uint16_t violationAddr         = 0;
};
