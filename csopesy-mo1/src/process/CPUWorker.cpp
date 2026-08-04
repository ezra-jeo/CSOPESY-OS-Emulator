#include "CPUWorker.h"
#include "IScheduler.h"
#include <chrono>
#include <thread>

CPUWorker::CPUWorker(int id, IScheduler& scheduler,
                     std::uint32_t quantum, std::uint32_t delaysPerExec,
                     IMemoryAllocator& allocator)
    : id(id), scheduler(scheduler), quantum(quantum), delaysPerExec(delaysPerExec),
      allocator(allocator) {}

CPUWorker::~CPUWorker() { stop(); }

void CPUWorker::start() {
    running = true;
    workerThread = std::thread(&CPUWorker::workerLoop, this);
}

void CPUWorker::stop() {
    running = false;
    cv.notify_all();
    if (workerThread.joinable()) workerThread.join();
}

void CPUWorker::assign(std::shared_ptr<Process> p) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        currentProcess = p;
        idle = false;
    }
    cv.notify_one();
}

bool CPUWorker::isIdle()    const { return idle; }
bool CPUWorker::isStalled() const { return stalled; }
int  CPUWorker::getId()     const { return id; }

std::shared_ptr<Process> CPUWorker::getCurrentProcess() const {
    std::lock_guard<std::mutex> lock(mtx);
    return currentProcess;
}

void CPUWorker::workerLoop() {
    while (true) {
        // STEP 1: sleep until assigned a process or told to stop
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]{ return !running || currentProcess != nullptr; });

        // STEP 2: clean shutdown — nothing left to do
        if (!running && currentProcess == nullptr) break;

        // STEP 3: take ownership and release the lock so assign() can run again
        auto proc = currentProcess;
        lock.unlock();

        // STEP 4: bind this core to the PCB
        proc->setCoreId(id);
        proc->setState(Process::RUNNING);

        // STEP 5: execute up to `quantum` instructions (0 = run to completion)
        std::uint32_t executed = 0;
        bool yielded = false;
        bool violated = false;
        while (!proc->isFinished()) {
            if (quantum > 0 && executed >= quantum) break; // quantum expired

            // Pace execution to the CPU clock: an instruction occupies max(1, delaysPerExec)
            // cycles — the spec's "one instruction per cycle" when delays-per-exec is 0, and
            // delays-per-exec cycles otherwise. Counting the delay as the instruction's whole
            // cost (rather than 1 + delay) keeps a process's total runtime comparable to the
            // batch-process-freq arrival interval, which is what lets a single-core, high-freq
            // configuration actually drain its ready queue and go idle. The tick is advanced by
            // the scheduler's free-running clock (SchedulerBase::watcherLoop).
            const std::uint64_t target =
                scheduler.getCpuTick() + (delaysPerExec ? delaysPerExec : 1);
            while (running.load() && scheduler.getCpuTick() < target)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if (!running.load()) break; // shutting down

            // A process whose address space needs more pages than physical memory has frames can
            // never hold a complete resident set: whatever it faults in must evict a page it still
            // needs. Model that as no forward progress — service one missing page and retry the
            // same instruction, forever. Processes that DO fit are untouched and keep ordinary
            // demand paging (one page faulted in per reference), so this only fires for a process
            // that is genuinely too large for the machine.
            if (proc->getPageCount() > allocator.frameCount()) {
                stalled = true;
                const std::uint64_t missing = proc->firstNonResidentPage();
                if (missing < proc->getPageCount())
                    allocator.handleFault(*proc, missing);
                continue; // no moveToNextLine(), no ++executed — nothing ever retires
            }
            stalled = false;

            proc->executeCurrentCommand();

            if (proc->hasViolation()) {
                violated = true;
                break;
            }

            if (proc->getFault() == Process::MemFault::PageFault) {
                allocator.handleFault(*proc, proc->getFaultPage());
                proc->clearFault();
                continue;   // do NOT moveToNextLine() or ++executed — the faulted instruction restarts.
                            // This deliberately does not count against the RR quantum: the spec describes
                            // fault resolution as happening "indefinitely" while the process still holds the
                            // core, not as consuming its fair share of instruction slots.
            }

            proc->moveToNextLine();
            ++executed;

            // SleepCommand sets a pending sleep request instead of blocking the thread.
            // We handle it here so the core is freed while the process waits.
            if (proc->hasSleepRequest()) {
                std::uint64_t wakeAt = scheduler.getCpuTick() + proc->getSleepTicks();
                proc->clearSleepRequest();
                proc->setState(Process::WAITING);
                scheduler.addToWaiting(proc, wakeAt);
                yielded = true;
                break;
            }
        }

        // STEP 6: finished vs. quantum-expired (preempted) vs. yielded for sleep
        if (yielded) {
            // Nothing to do — watcher thread re-admits via requeueReady when tick expires.
        } else if (violated) {
            proc->setState(Process::FINISHED);
            scheduler.moveToFinished(proc);
        } else if (proc->isFinished()) {
            proc->setState(Process::FINISHED);
            scheduler.moveToFinished(proc);
        } else {
            // Quantum expired with work remaining — requeue at the tail (RR preemption).
            proc->setState(Process::READY);
            scheduler.requeue(proc);
        }

        // STEP 7: clear slot and become idle
        {
            std::lock_guard<std::mutex> lg(mtx);
            currentProcess = nullptr;
        }
        stalled = false;
        idle = true;

        // STEP 8: wake the scheduler so it can dispatch the next process
        scheduler.notifyScheduler();
    }
}
