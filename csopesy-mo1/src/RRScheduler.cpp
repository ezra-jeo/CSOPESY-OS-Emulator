#include "RRScheduler.h"
#include "CPUWorker.h"

RRScheduler::RRScheduler(int numCores, std::uint32_t quantumCycles, std::uint32_t delaysPerExec)
    : numCores(numCores), quantumCycles(quantumCycles), delaysPerExec(delaysPerExec) {
    workers.reserve(numCores);
    for (int i = 0; i < numCores; ++i)
        workers.push_back(std::make_unique<CPUWorker>(i, *this, quantumCycles, delaysPerExec));
}

RRScheduler::~RRScheduler() { stop(); }

void RRScheduler::addProcess(std::shared_ptr<Process> p) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(p);
    }
    schedulerCv.notify_one();
}

void RRScheduler::requeue(std::shared_ptr<Process> p) {
    // Preemption path: push to tail so every ready process gets a fair turn.
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(std::move(p));
    }
    schedulerCv.notify_one();
}

void RRScheduler::requeueReady(std::shared_ptr<Process> p) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(std::move(p));
    }
    schedulerCv.notify_one();
}

void RRScheduler::start() {
    running = true;
    for (auto& w : workers) w->start();
    schedulerThread = std::thread(&RRScheduler::schedulerLoop, this);
    startWatcher();
}

void RRScheduler::stop() {
    stopWatcher();
    running = false;
    schedulerCv.notify_all();
    if (schedulerThread.joinable()) schedulerThread.join();
    for (auto& w : workers) w->stop();
}

void RRScheduler::moveToFinished(std::shared_ptr<Process> p) {
    std::lock_guard<std::mutex> lock(finishedMutex);
    finishedList.push_back(p);
}

void RRScheduler::notifyScheduler() {
    schedulerCv.notify_one();
}

std::vector<std::shared_ptr<Process>> RRScheduler::getRunningProcesses() const {
    std::vector<std::shared_ptr<Process>> result;
    for (const auto& w : workers) {
        auto p = w->getCurrentProcess();
        if (p) result.push_back(p);
    }
    return result;
}

std::vector<std::shared_ptr<Process>> RRScheduler::getFinishedProcesses() const {
    std::lock_guard<std::mutex> lock(finishedMutex);
    return finishedList;
}

int RRScheduler::getNumCores() const { return numCores; }

int RRScheduler::getActiveCores() const {
    int count = 0;
    for (const auto& w : workers) if (!w->isIdle()) ++count;
    return count;
}

void RRScheduler::schedulerLoop() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        schedulerCv.wait(lock, [&] {
            bool hasWork     = !readyQueue.empty();
            bool hasFreeCore = false;
            for (auto& w : workers)
                if (w->isIdle()) { hasFreeCore = true; break; }
            return (!running && !hasWork) || (hasWork && hasFreeCore);
        });

        if (!running && readyQueue.empty()) break;

        CPUWorker* idle = nullptr;
        for (auto& w : workers)
            if (w->isIdle()) { idle = w.get(); break; }
        if (!idle) continue; // spurious wake

        auto proc = readyQueue.front();
        readyQueue.pop();
        lock.unlock();

        idle->assign(proc);
    }
}
