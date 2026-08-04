#include "SchedulerBase.h"
#include "Process.h"
#include <algorithm>
#include <chrono>
#include <thread>

// Wall-clock duration of one CPU cycle (tick). Workers pace execution to this clock — each
// instruction consumes (1 + delays-per-exec) cycles — so processes advance at an observable rate
// even when delays-per-exec is 0. Also the unit for SLEEP ticks and batch-process-freq. Tunable.
//
// Kept well above CPUWorker's own 1ms poll granularity (see CPUWorker::workerLoop's pacing loop)
// so the tick, not poll jitter, is what actually paces execution. At 5ms/tick, a fixed wall-clock
// window (e.g. a timed quiz scenario waiting N real seconds) admits far more instruction
// executions than the previous 200ms/tick allowed — needed for demand-paging thrash scenarios
// where a very high paged-in/paged-out count is expected within a short, fixed real-time window.
namespace {
    constexpr int CPU_CYCLE_MS = 5;

    // Defensive floor only — every process already gets a real requested size (via
    // Process::setRequestedMemSize) before it ever reaches acquireMemory, so this is not an
    // expected code path. Matches the spec's minimum process footprint.
    constexpr std::uint64_t kMinProcessMemory = 64;
}

SchedulerBase::SchedulerBase(IMemoryAllocator& memory, std::uint32_t quantumCycles)
    : memory(memory), quantumCycles(quantumCycles) {}

bool SchedulerBase::acquireMemory(const std::shared_ptr<Process>& p) {
    if (p->hasMemory()) return true;
    std::uint64_t size = p->getRequestedMemSize();
    if (size == 0) size = kMinProcessMemory;
    return memory.admit(*p, size);
}

void SchedulerBase::releaseMemory(const std::shared_ptr<Process>& p) {
    memory.deallocate(p->getName());
}

void SchedulerBase::addToWaiting(std::shared_ptr<Process> p, std::uint64_t wakeAtTick) {
    std::lock_guard<std::mutex> lk(waitingMutex);
    waitingList.push_back({wakeAtTick, std::move(p)});
}

void SchedulerBase::incrementTick() {
    cpuTick.fetch_add(1, std::memory_order_relaxed);
}

std::uint64_t SchedulerBase::getCpuTick() const {
    return cpuTick.load(std::memory_order_relaxed);
}

std::uint64_t SchedulerBase::getIdleTicks() const {
    return idleTicks.load(std::memory_order_relaxed);
}

std::uint64_t SchedulerBase::getActiveTicks() const {
    return activeTicks.load(std::memory_order_relaxed);
}

std::uint64_t SchedulerBase::getTotalTicks() const {
    return totalTicks.load(std::memory_order_relaxed);
}

void SchedulerBase::startWatcher() {
    watcherRunning = true;
    watcherThread = std::thread(&SchedulerBase::watcherLoop, this);
}

void SchedulerBase::stopWatcher() {
    watcherRunning = false;
    if (watcherThread.joinable()) watcherThread.join();
}

void SchedulerBase::watcherLoop() {
    using namespace std::chrono_literals;
    while (watcherRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(CPU_CYCLE_MS));

        // Free-running CPU clock (spec: `while(running) cpuCycles++`). Advancing the tick here —
        // rather than per executed instruction — keeps it moving even when every process is
        // sleeping or the ready queue is momentarily empty, so SLEEP timers and tick-driven batch
        // generation can never stall the system.
        incrementTick();

        // getNumCores()/getActiveCores() are pure virtuals from IScheduler; watcherLoop runs on a
        // fully-constructed FCFSScheduler/RRScheduler (the watcher thread only starts inside
        // start()), so these ordinary virtual calls correctly dispatch to the concrete override.
        int total  = getNumCores();
        int active = getActiveCores();
        activeTicks.fetch_add(static_cast<std::uint64_t>(active), std::memory_order_relaxed);
        idleTicks.fetch_add(static_cast<std::uint64_t>(total - active), std::memory_order_relaxed);
        totalTicks.fetch_add(static_cast<std::uint64_t>(total), std::memory_order_relaxed);

        std::vector<std::shared_ptr<Process>> toWake;
        {
            std::lock_guard<std::mutex> lk(waitingMutex);
            std::uint64_t now = cpuTick.load(std::memory_order_relaxed);
            auto it = std::stable_partition(
                waitingList.begin(), waitingList.end(),
                [now](const WaitEntry& e) { return e.wakeAt > now; });
            for (auto p = it; p != waitingList.end(); ++p)
                toWake.push_back(std::move(p->proc));
            waitingList.erase(it, waitingList.end());
        }
        for (auto& p : toWake) {
            p->setState(Process::READY);
            requeueReady(std::move(p));
        }
    }
}
