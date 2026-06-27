#include "SchedulerBase.h"
#include "Process.h"
#include <algorithm>
#include <chrono>
#include <thread>

// Wall-clock duration of one CPU cycle (tick). Workers pace execution to this clock — each
// instruction consumes (1 + delays-per-exec) cycles — so processes advance at an observable rate
// even when delays-per-exec is 0. Also the unit for SLEEP ticks and batch-process-freq. Tunable.
namespace { constexpr int CPU_CYCLE_MS = 200; }

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
