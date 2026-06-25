#pragma once
#include "ICommand.h"
#include "SymbolTable.h"
#include <string>
#include <vector>
#include <memory>
#include <ctime>

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
};
