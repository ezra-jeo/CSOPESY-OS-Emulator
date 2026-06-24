#include "FCFSScheduler.h"
#include "CPUWorker.h"

FCFSScheduler::FCFSScheduler(int numCores, std::uint32_t delaysPerExec)
    : numCores(numCores), delaysPerExec(delaysPerExec) {
    workers.reserve(numCores);
    for (int i = 0; i < numCores; ++i)
        workers.push_back(std::make_unique<CPUWorker>(i, *this, /*quantum=*/0, delaysPerExec));
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
    // quantum=0 means workers run to completion, so this is never called in practice.
    // Provided for IScheduler conformance.
    addProcess(p);
}

void FCFSScheduler::start() {
    running = true;
    for (auto& w : workers) w->start();
    schedulerThread = std::thread(&FCFSScheduler::schedulerLoop, this);
}

void FCFSScheduler::stop() {
    running = false;
    schedulerCv.notify_all();
    if (schedulerThread.joinable()) schedulerThread.join();
    for (auto& w : workers) w->stop();
}

void FCFSScheduler::moveToFinished(std::shared_ptr<Process> p) {
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
        // Wait until there is work AND a free core, or until shutdown with empty queue.
        std::unique_lock<std::mutex> lock(queueMutex);
        schedulerCv.wait(lock, [&] {
            bool hasWork     = !readyQueue.empty();
            bool hasFreeCore = false;
            for (auto& w : workers)
                if (w->isIdle()) { hasFreeCore = true; break; }
            return (!running && !hasWork) || (hasWork && hasFreeCore);
        });

        if (!running && readyQueue.empty()) break;

        // Find first idle worker
        CPUWorker* idle = nullptr;
        for (auto& w : workers)
            if (w->isIdle()) { idle = w.get(); break; }
        if (!idle) continue; // spurious wake

        // Pop the front — FCFS means arrival order, never sorted
        auto proc = readyQueue.front();
        readyQueue.pop();
        lock.unlock(); // release before assign() to avoid holding two locks at once

        idle->assign(proc);
    }
}
