#include "FCFSScheduler.h"
#include "CPUWorker.h"
#include <chrono>
#include <thread>

FCFSScheduler::FCFSScheduler(int numCores, std::uint32_t delaysPerExec,
                              IMemoryAllocator& memory, std::uint32_t quantumCycles)
    : SchedulerBase(memory, quantumCycles),
      numCores(numCores), delaysPerExec(delaysPerExec) {
    workers.reserve(numCores);
    for (int i = 0; i < numCores; ++i)
        workers.push_back(std::make_unique<CPUWorker>(i, *this, /*quantum=*/0, delaysPerExec, memory));
}

FCFSScheduler::~FCFSScheduler() { stop(); }

void FCFSScheduler::addProcess(std::shared_ptr<Process> p) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(p);
    }
    schedulerCv.notify_one();
}

void FCFSScheduler::requeue(std::shared_ptr<Process> p) {
    addProcess(p);
}

void FCFSScheduler::requeueReady(std::shared_ptr<Process> p) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(std::move(p));
    }
    schedulerCv.notify_one();
}

void FCFSScheduler::start() {
    running = true;
    for (auto& w : workers) w->start();
    schedulerThread = std::thread(&FCFSScheduler::schedulerLoop, this);
    startWatcher();
}

void FCFSScheduler::stop() {
    stopWatcher();
    running = false;
    schedulerCv.notify_all();
    if (schedulerThread.joinable()) schedulerThread.join();
    for (auto& w : workers) w->stop();
}

void FCFSScheduler::moveToFinished(std::shared_ptr<Process> p) {
    // Memory is only released once the process finishes execution.
    releaseMemory(p);
    std::lock_guard<std::mutex> lock(finishedMutex);
    finishedList.push_back(p);
}

void FCFSScheduler::notifyScheduler() {
    schedulerCv.notify_one();
}

std::vector<std::shared_ptr<Process>> FCFSScheduler::getRunningProcesses() const {
    std::vector<std::shared_ptr<Process>> result;
    for (const auto& w : workers) {
        auto p = w->getCurrentProcess();
        if (p) result.push_back(p);
    }
    return result;
}

std::vector<std::shared_ptr<Process>> FCFSScheduler::getFinishedProcesses() const {
    std::lock_guard<std::mutex> lock(finishedMutex);
    return finishedList;
}

int FCFSScheduler::getNumCores()    const { return numCores; }

int FCFSScheduler::getActiveCores() const {
    int count = 0;
    for (const auto& w : workers) if (!w->isIdle()) ++count;
    return count;
}

void FCFSScheduler::schedulerLoop() {
    while (true) {
        // Wait until there is work AND a free core, or until shutdown.
        std::unique_lock<std::mutex> lock(queueMutex);
        schedulerCv.wait(lock, [&] {
            if (!running) return true; // shutdown: stop waiting immediately, don't drain the queue
            bool hasWork     = !readyQueue.empty();
            bool hasFreeCore = false;
            for (auto& w : workers)
                if (w->isIdle()) { hasFreeCore = true; break; }
            return hasWork && hasFreeCore;
        });

        // stop() must be able to close the emulator promptly even with a large backlog of
        // memory-starved processes still queued — exit abandons anything not already
        // dispatched to a core.
        if (!running) break;

        // Find first idle worker
        CPUWorker* idle = nullptr;
        for (auto& w : workers)
            if (w->isIdle()) { idle = w.get(); break; }
        if (!idle) continue; // spurious wake

        // Pop the front — FCFS means arrival order, never sorted — but skip (revert to tail)
        // anything that can't secure memPerProc bytes right now ("if memory is full when a
        // process is scheduled, it reverts to the tail of the ready queue").
        std::shared_ptr<Process> proc;
        for (std::size_t attempts = readyQueue.size(); attempts > 0; --attempts) {
            auto candidate = readyQueue.front();
            readyQueue.pop();
            if (acquireMemory(candidate)) { proc = candidate; break; }
            readyQueue.push(candidate);
        }

        if (!proc) {
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(5)); // avoid busy-spin while full
            continue;
        }

        lock.unlock(); // release before assign() to avoid holding two locks at once
        idle->assign(proc);
    }
}
