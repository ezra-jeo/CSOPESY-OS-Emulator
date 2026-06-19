#include "FCFSScheduler.h"
#include "CPUWorker.h"  // full header needed to call worker methods

// ── Constructor / destructor ──────────────────────────────────────────────────

FCFSScheduler::FCFSScheduler(int numCores) : numCores(numCores) {
    workers.reserve(numCores);
    for (int i = 0; i < numCores; ++i)
        workers.push_back(std::make_unique<CPUWorker>(i, *this));
}

FCFSScheduler::~FCFSScheduler() { stop(); }

// ── addProcess ────────────────────────────────────────────────────────────────
// Long-term scheduler: admit a process into the ready queue in arrival order.
// FCFS = FIFO, so we always push to the back.
void FCFSScheduler::addProcess(std::shared_ptr<Process> p) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(p);  // preserve arrival order — do NOT sort
    }
    schedulerCv.notify_one(); // wake schedulerLoop in case a core is already idle
}

// ── Thread lifecycle ──────────────────────────────────────────────────────────

void FCFSScheduler::start() {
    running = true;
    for (auto& w : workers)       w->start();
    schedulerThread = std::thread(&FCFSScheduler::schedulerLoop, this);
}

void FCFSScheduler::stop() {
    running = false;
    schedulerCv.notify_all();
    if (schedulerThread.joinable()) schedulerThread.join();
    for (auto& w : workers)       w->stop(); // stop workers after scheduler exits
}

// ── moveToFinished ────────────────────────────────────────────────────────────
// Called by a worker after it completes all instructions of a process.
void FCFSScheduler::moveToFinished(std::shared_ptr<Process> p) {
    std::lock_guard<std::mutex> lock(finishedMutex);
    finishedList.push_back(p);
}

// ── notifyScheduler ───────────────────────────────────────────────────────────
// Workers call this when they become idle so schedulerLoop can dispatch next.
void FCFSScheduler::notifyScheduler() {
    schedulerCv.notify_one();
}

// ── Console snapshots ─────────────────────────────────────────────────────────

std::vector<std::shared_ptr<Process>> FCFSScheduler::getRunningProcesses() const {
    // A process is "running" if it is currently held by a worker.
    // NOTE: reading process fields (commandCounter, etc.) without a per-process
    //       lock can produce torn values if the worker is mid-update.  Acceptable
    //       for Phase 1 (Console is for human inspection, not for correctness checks).
    std::vector<std::shared_ptr<Process>> result;
    for (const auto& w : workers) {
        auto p = w->getCurrentProcess();
        if (p) result.push_back(p);
    }
    return result;
}

std::vector<std::shared_ptr<Process>> FCFSScheduler::getFinishedProcesses() const {
    std::lock_guard<std::mutex> lock(finishedMutex);
    return finishedList; // returns a copy — safe to read without holding the lock after return
}

int FCFSScheduler::getNumCores() const { return numCores; }

int FCFSScheduler::getActiveCores() const {
    int count = 0;
    for (const auto& w : workers) if (!w->isIdle()) ++count;
    return count;
}

// ── schedulerLoop ─────────────────────────────────────────────────────────────
// TODO(student): implement this function.
//
// This is the short-term scheduler (lecture §4 – selects the next process from
// the ready queue and dispatches it to a free CPU core).
//
// Outer loop: while (running || !readyQueue.empty())  — drain the queue even
//             after stop() is called, so processes in flight finish cleanly.
//
// STEP 1: Acquire queueMutex and block on schedulerCv until:
//           • running == false AND readyQueue is empty (shutdown with no work), OR
//           • readyQueue is non-empty AND at least one worker isIdle()
//         Use:
//           std::unique_lock<std::mutex> lock(queueMutex);
//           schedulerCv.wait(lock, [&]{
//               bool hasWork = !readyQueue.empty();
//               bool hasFreeCore = false;
//               for (auto& w : workers) if (w->isIdle()) { hasFreeCore = true; break; }
//               return (!running && !hasWork) || (hasWork && hasFreeCore);
//           });
//
// STEP 2: Check exit condition AFTER the wait:
//           if (!running && readyQueue.empty()) break;
//
// STEP 3: Find the first idle worker:
//           CPUWorker* idle = nullptr;
//           for (auto& w : workers) if (w->isIdle()) { idle = w.get(); break; }
//           if (!idle) continue;  // spurious wake — no idle core yet
//
// STEP 4: Pop the FRONT of the queue (FCFS — never peek at burst time, never sort):
//           auto proc = readyQueue.front();
//           readyQueue.pop();
//
// STEP 5: Release the lock BEFORE calling assign() to avoid holding two locks:
//           lock.unlock();
//
// STEP 6: Dispatch:
//           idle->assign(proc);
//
// STEP 7: Loop back to STEP 1.
void FCFSScheduler::schedulerLoop() {
    // TODO(student): implement using the steps above.
}
