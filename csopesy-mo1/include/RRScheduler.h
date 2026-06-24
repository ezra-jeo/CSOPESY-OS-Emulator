#pragma once
#include "Process.h"
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <thread>

// Round-robin scheduler skeleton (MO1 `scheduler rr`). Mirrors FCFSScheduler's public
// surface so Console can drive either policy. The key difference is preemption: a
// running process is interrupted after `quantumCycles` CPU ticks and requeued at the
// tail of the ready queue.
//
// NOTE: CPUWorker is currently bound to FCFSScheduler& by its constructor. To share the
// worker pool you'll either (a) generalize CPUWorker over a scheduler interface, or
// (b) give RR its own worker loop. Left as a design decision for the implementer.
class RRScheduler {
public:
    RRScheduler(int numCores, std::uint32_t quantumCycles);
    ~RRScheduler();

    void addProcess(std::shared_ptr<Process> p);
    void start();
    void stop();

    void moveToFinished(std::shared_ptr<Process> p);
    void notifyScheduler();

    std::vector<std::shared_ptr<Process>> getRunningProcesses()  const;
    std::vector<std::shared_ptr<Process>> getFinishedProcesses() const;
    int getNumCores()    const;
    int getActiveCores() const;

private:
    void schedulerLoop(); // TODO(student): dispatch + quantum-based preemption + requeue

    int           numCores;
    std::uint32_t quantumCycles;

    std::queue<std::shared_ptr<Process>> readyQueue;
    mutable std::mutex                   queueMutex;
    std::condition_variable              schedulerCv;

    std::atomic<bool> running{false};

    std::vector<std::shared_ptr<Process>> finishedList;
    mutable std::mutex                    finishedMutex;

    std::thread schedulerThread;
};
