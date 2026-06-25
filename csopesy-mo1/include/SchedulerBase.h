#pragma once
#include "IScheduler.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

// Intermediate base class shared by FCFSScheduler and RRScheduler.
// Provides the CPU tick counter, waiting list, and a watcher thread that
// re-admits sleeping processes once their tick expiry is reached.
class SchedulerBase : public IScheduler {
protected:
    void startWatcher();
    void stopWatcher();

    // Subclass pushes p to its own ready queue and notifies its scheduler CV.
    virtual void requeueReady(std::shared_ptr<Process> p) = 0;

public:
    void          addToWaiting(std::shared_ptr<Process> p, std::uint64_t wakeAtTick) override;
    void          incrementTick()    override;
    std::uint64_t getCpuTick() const override;

private:
    void watcherLoop();

    std::atomic<std::uint64_t> cpuTick{0};

    struct WaitEntry { std::uint64_t wakeAt; std::shared_ptr<Process> proc; };
    std::vector<WaitEntry> waitingList;
    mutable std::mutex     waitingMutex;

    std::atomic<bool> watcherRunning{false};
    std::thread       watcherThread;
};
