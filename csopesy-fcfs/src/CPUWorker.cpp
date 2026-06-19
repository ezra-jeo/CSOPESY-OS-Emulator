#include "CPUWorker.h"
#include "FCFSScheduler.h"  // full header needed to call scheduler.moveToFinished() etc.

// ── Constructor / destructor ──────────────────────────────────────────────────

CPUWorker::CPUWorker(int id, FCFSScheduler& scheduler)
    : id(id), scheduler(scheduler) {}

CPUWorker::~CPUWorker() { stop(); }

// ── Thread lifecycle ──────────────────────────────────────────────────────────

void CPUWorker::start() {
    running = true;
    workerThread = std::thread(&CPUWorker::workerLoop, this);
}

void CPUWorker::stop() {
    running = false;
    cv.notify_all();
    if (workerThread.joinable()) workerThread.join();
}

// ── assign ────────────────────────────────────────────────────────────────────
// Called by FCFSScheduler::schedulerLoop() to hand a ready process to this core.
// Sets the current-process slot, marks the core busy, then wakes workerLoop().
void CPUWorker::assign(std::shared_ptr<Process> p) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        currentProcess = p;
        idle = false;
    }
    cv.notify_one();
}

// ── Utility ───────────────────────────────────────────────────────────────────

bool CPUWorker::isIdle() const { return idle; }
int  CPUWorker::getId()  const { return id; }

std::shared_ptr<Process> CPUWorker::getCurrentProcess() const {
    std::lock_guard<std::mutex> lock(mtx);
    return currentProcess;
}

// ── workerLoop ────────────────────────────────────────────────────────────────
// TODO(student): implement this function.
//
// This is the core of the CPU simulation (lecture §4 – processor executing
// instructions from the PCB's command list until the process finishes).
//
// STEP 1: Acquire mtx and wait on `cv` until EITHER:
//           • running == false  (shutdown signal from stop()), OR
//           • currentProcess != nullptr  (a process was assigned via assign())
//         Use:
//           std::unique_lock<std::mutex> lock(mtx);
//           cv.wait(lock, [&]{ return !running || currentProcess != nullptr; });
//
// STEP 2: If running == false AND currentProcess == nullptr, break out of the
//         outer while-loop (clean shutdown with nothing left to do).
//
// STEP 3: Grab a local copy of the assigned process and release the lock early
//         so assign() can be called again without blocking:
//           auto proc = currentProcess;   // local shared_ptr keeps it alive
//           lock.unlock();
//
// STEP 4: Configure the PCB for execution on this core:
//           proc->setCoreId(id);
//           proc->setState(Process::RUNNING);
//
// STEP 5: Non-preemptive execution — run ALL commands to completion before
//         yielding (this is what makes FCFS non-preemptive):
//           while (!proc->isFinished()) {
//               proc->executeCurrentCommand();   // runs the instruction + EXEC_DELAY_MS
//               proc->moveToNextLine();
//           }
//
// STEP 6: Mark the process done:
//           proc->setState(Process::FINISHED);
//
// STEP 7: Hand the finished process to the scheduler's finished list:
//           scheduler.moveToFinished(proc);
//
// STEP 8: Reset this core's slot and mark it idle again:
//           { std::lock_guard<std::mutex> lg(mtx); currentProcess = nullptr; }
//           idle = true;
//
// STEP 9: Wake the scheduler so it can dispatch the next waiting process:
//           scheduler.notifyScheduler();
//
// STEP 10: Loop back to STEP 1 and wait for the next assignment.
void CPUWorker::workerLoop() {
    // TODO(student): implement using the steps above.
    (void)id; // suppress unused-variable warning while stub is in place
}
