#pragma once
#include "Process.h"
#include <memory>
#include <vector>

// Abstract scheduler interface shared by FCFSScheduler and RRScheduler.
// CPUWorker holds an IScheduler& so it can call back without knowing the policy.
class IScheduler {
public:
    virtual ~IScheduler() = default;

    // Long-term admission: enqueue a new (READY) process.
    virtual void addProcess(std::shared_ptr<Process> p) = 0;

    // Re-admit a preempted process to the tail of the ready queue (RR).
    // FCFS implements this as a no-op / identical to addProcess (never called for FCFS).
    virtual void requeue(std::shared_ptr<Process> p) = 0;

    virtual void start() = 0;
    virtual void stop()  = 0;

    // Called by a worker when it finishes executing all commands of a process.
    virtual void moveToFinished(std::shared_ptr<Process> p) = 0;

    // Called by a worker when it becomes idle so the scheduler loop can wake up.
    virtual void notifyScheduler() = 0;

    virtual std::vector<std::shared_ptr<Process>> getRunningProcesses()  const = 0;
    virtual std::vector<std::shared_ptr<Process>> getFinishedProcesses() const = 0;
    virtual int getNumCores()    const = 0;
    virtual int getActiveCores() const = 0;
};
