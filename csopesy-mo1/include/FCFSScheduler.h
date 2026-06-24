#pragma once
#include "Process.h"
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <thread>

class CPUWorker; // forward declaration — avoids circular include with CPUWorker.h

// Combines the long-term scheduler (admits processes into the ready queue) and
// the short-term scheduler (dispatches the front of the FCFS queue to idle cores).
// Ready queue = FIFO; arrival order is preserved by always pushing to the back.
class FCFSScheduler {
public:
    explicit FCFSScheduler(int numCores);
    ~FCFSScheduler();

    // Long-term admission: enqueue a newly created process (call before start()).
    void addProcess(std::shared_ptr<Process> p);

    void start(); // launch scheduler thread + all worker threads
    void stop();  // signal shutdown, drain, join all threads

    // Called by a worker when it finishes executing all commands of a process.
    void moveToFinished(std::shared_ptr<Process> p);

    // Called by a worker when it becomes idle so the scheduler loop can wake up.
    void notifyScheduler();

    // Read-only snapshots for Console::printProcessList() — each acquires its lock.
    std::vector<std::shared_ptr<Process>> getRunningProcesses()  const;
    std::vector<std::shared_ptr<Process>> getFinishedProcesses() const;
    int getNumCores()    const;
    int getActiveCores() const; // number of cores currently executing a process

private:
    // TODO(student): implement schedulerLoop() — see step-by-step comments in FCFSScheduler.cpp
    void schedulerLoop();

    int numCores;

    // Ready queue (lecture §4 – FCFS = FIFO; do NOT sort, do NOT reorder).
    std::queue<std::shared_ptr<Process>> readyQueue;
    mutable std::mutex                   queueMutex;
    std::condition_variable              schedulerCv;

    std::atomic<bool> running{false};

    std::vector<std::unique_ptr<CPUWorker>> workers; // one entry per core

    std::vector<std::shared_ptr<Process>> finishedList;
    mutable std::mutex                    finishedMutex;

    std::thread schedulerThread;
};
