#pragma once
#include "SchedulerBase.h"
#include "Process.h"
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <thread>
#include <cstdint>

class CPUWorker; // forward declaration

// Round-robin scheduler. Mirrors FCFSScheduler's structure; the key difference is
// that workers are built with quantum=quantumCycles so they preempt after N instructions
// and call back via requeue() to push the process to the tail of the ready queue.
class RRScheduler : public SchedulerBase {
public:
    RRScheduler(int numCores, std::uint32_t quantumCycles, std::uint32_t delaysPerExec = 0);
    ~RRScheduler() override;

    void addProcess(std::shared_ptr<Process> p) override;
    void requeue(std::shared_ptr<Process> p)    override; // push to tail (preemption path)
    void start() override;
    void stop()  override;

    void moveToFinished(std::shared_ptr<Process> p) override;
    void notifyScheduler()                          override;

    std::vector<std::shared_ptr<Process>> getRunningProcesses()  const override;
    std::vector<std::shared_ptr<Process>> getFinishedProcesses() const override;
    int getNumCores()    const override;
    int getActiveCores() const override;

    void requeueReady(std::shared_ptr<Process> p) override; // SchedulerBase hook

private:
    void schedulerLoop();

    int           numCores;
    std::uint32_t quantumCycles;
    std::uint32_t delaysPerExec;

    std::queue<std::shared_ptr<Process>> readyQueue;
    mutable std::mutex                   queueMutex;
    std::condition_variable              schedulerCv;

    std::atomic<bool> running{false};

    std::vector<std::unique_ptr<CPUWorker>> workers;

    std::vector<std::shared_ptr<Process>> finishedList;
    mutable std::mutex                    finishedMutex;

    std::thread schedulerThread;
};
