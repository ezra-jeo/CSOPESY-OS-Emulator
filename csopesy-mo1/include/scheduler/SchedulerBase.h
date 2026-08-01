#pragma once
#include "IScheduler.h"
#include "IMemoryAllocator.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

// Intermediate base class shared by FCFSScheduler and RRScheduler.
// Provides the CPU tick counter, waiting list, a watcher thread that re-admits sleeping
// processes once their tick expiry is reached, and the first-fit memory allocator hooks
// (acquireMemory/releaseMemory) both scheduling policies dispatch through.
class SchedulerBase : public IScheduler {
public:
    SchedulerBase(IMemoryAllocator& memory, std::uint64_t memPerProc, std::uint32_t quantumCycles);

protected:
    void startWatcher();
    void stopWatcher();

    // Subclass pushes p to its own ready queue and notifies its scheduler CV.
    virtual void requeueReady(std::shared_ptr<Process> p) = 0;

    // Secures memPerProc bytes for p (no-op if p already holds memory from an earlier
    // quantum). Returns false if the memory manager has no block large enough right now —
    // the caller should push p back onto the tail of its ready queue and try another candidate
    // ("if memory is full when a process is scheduled, it reverts to the tail of the queue").
    bool acquireMemory(const std::shared_ptr<Process>& p);

    // Releases p's memory block. Call once, when p finishes (not on quantum preemption).
    void releaseMemory(const std::shared_ptr<Process>& p);

public:
    void          addToWaiting(std::shared_ptr<Process> p, std::uint64_t wakeAtTick) override;
    void          incrementTick()    override;
    std::uint64_t getCpuTick() const override;

    std::uint64_t getIdleTicks()   const override;
    std::uint64_t getActiveTicks() const override;
    std::uint64_t getTotalTicks()  const override;

private:
    void watcherLoop();

    std::atomic<std::uint64_t> cpuTick{0};

    // vmstat tick accounting (see IScheduler::getIdleTicks/getActiveTicks/getTotalTicks).
    std::atomic<std::uint64_t> idleTicks{0};
    std::atomic<std::uint64_t> activeTicks{0};
    std::atomic<std::uint64_t> totalTicks{0};

    struct WaitEntry { std::uint64_t wakeAt; std::shared_ptr<Process> proc; };
    std::vector<WaitEntry> waitingList;
    mutable std::mutex     waitingMutex;

    std::atomic<bool> watcherRunning{false};
    std::thread       watcherThread;

    IMemoryAllocator& memory;
    std::uint64_t  memPerProc;
    std::uint32_t  quantumCycles;      // also the memory-snapshot cadence, in CPU ticks
    int            quantumSnapshotIndex = 0;
    std::uint32_t  ticksSinceSnapshot   = 0;
};
