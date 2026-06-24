#pragma once
#include "IScheduler.h"
#include "Process.h"
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <thread>
#include <cstdint>

class CPUWorker; // forward declaration — avoids circular include with CPUWorker.h

// Combines the long-term scheduler (admits processes into the ready queue) and
// the short-term scheduler (dispatches the front of the FCFS queue to idle cores).
// Ready queue = FIFO; arrival order is preserved by always pushing to the back.
class FCFSScheduler : public IScheduler {
public:
    FCFSScheduler(int numCores, std::uint32_t delaysPerExec = 0);
    ~FCFSScheduler() override;

    void addProcess(std::shared_ptr<Process> p) override;
    void requeue(std::shared_ptr<Process> p)    override; // same as addProcess for FCFS
    void start() override;
    void stop()  override;

    void moveToFinished(std::shared_ptr<Process> p) override;
    void notifyScheduler()                          override;

    std::vector<std::shared_ptr<Process>> getRunningProcesses()  const override;
    std::vector<std::shared_ptr<Process>> getFinishedProcesses() const override;
    int getNumCores()    const override;
    int getActiveCores() const override;

private:
    void schedulerLoop();

    int           numCores;
    std::uint32_t delaysPerExec;

    std::queue<std::shared_ptr<Process>> readyQueue;
    mutable std::mutex                   queueMutex;
    std::condition_variable              schedulerCv;

    std::atomic<bool> running{false};

    std::vector<std::unique_ptr<CPUWorker>> workers; // one entry per core

    std::vector<std::shared_ptr<Process>> finishedList;
    mutable std::mutex                    finishedMutex;

    std::thread schedulerThread;
};
