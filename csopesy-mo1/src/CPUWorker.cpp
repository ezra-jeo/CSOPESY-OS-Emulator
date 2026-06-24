#include "CPUWorker.h"
#include "IScheduler.h"
#include <chrono>
#include <thread>

CPUWorker::CPUWorker(int id, IScheduler& scheduler,
                     std::uint32_t quantum, std::uint32_t delaysPerExec)
    : id(id), scheduler(scheduler), quantum(quantum), delaysPerExec(delaysPerExec) {}

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

bool CPUWorker::isIdle() const { return idle; }
int  CPUWorker::getId()  const { return id; }

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
        while (!proc->isFinished()) {
            if (quantum > 0 && executed >= quantum) break; // quantum expired

            // Extra delay before each instruction (config delays-per-exec; 0 = none).
            // Note: PrintCommand also has its own seed delay via Config::EXEC_DELAY_MS.
            if (delaysPerExec > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(delaysPerExec));

            proc->executeCurrentCommand();
            proc->moveToNextLine();
            ++executed;
        }

        // STEP 6: finished vs. quantum-expired (preempted)
        if (proc->isFinished()) {
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
        idle = true;

        // STEP 8: wake the scheduler so it can dispatch the next process
        scheduler.notifyScheduler();
    }
}
