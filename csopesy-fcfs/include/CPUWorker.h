#pragma once
#include "Process.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

class FCFSScheduler; // forward declaration — avoids circular include with FCFSScheduler.h

// One CPUWorker represents one CPU core (lecture §4 – processor data structure).
// The short-term scheduler hands ready processes to idle workers via assign().
class CPUWorker {
public:
    CPUWorker(int id, FCFSScheduler& scheduler);
    ~CPUWorker();

    void start(); // launch the worker thread
    void stop();  // signal shutdown and join

    // Short-term scheduler calls this to give the worker a ready process.
    void assign(std::shared_ptr<Process> p);

    bool                     isIdle()            const;
    int                      getId()             const;
    std::shared_ptr<Process> getCurrentProcess() const; // snapshot for Console

private:
    // TODO(student): implement workerLoop() — see step-by-step comments in CPUWorker.cpp
    void workerLoop();

    int            id;
    FCFSScheduler& scheduler; // back-pointer so the worker can report to the scheduler

    std::shared_ptr<Process>  currentProcess;
    std::atomic<bool>         running{false};
    std::atomic<bool>         idle{true};
    mutable std::mutex        mtx;
    std::condition_variable   cv;
    std::thread               workerThread;
};
