#include "CPUWorker.h"
#include "FCFSScheduler.h"

CPUWorker::CPUWorker(int id, FCFSScheduler& scheduler)
    : id(id), scheduler(scheduler) {}

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

        // STEP 5: non-preemptive — run every instruction before yielding
        while (!proc->isFinished()) {
            proc->executeCurrentCommand();
            proc->moveToNextLine();
        }

        // STEP 6: mark finished
        proc->setState(Process::FINISHED);

        // STEP 7: hand off to finished list
        scheduler.moveToFinished(proc);

        // STEP 8: clear slot and become idle
        {
            std::lock_guard<std::mutex> lg(mtx);
            currentProcess = nullptr;
        }
        idle = true;

        // STEP 9: wake the scheduler so it can dispatch the next process
        scheduler.notifyScheduler();
    }
}
