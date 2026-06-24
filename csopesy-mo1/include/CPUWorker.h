#pragma once
#include "Process.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <cstdint>

class IScheduler; // forward declaration — avoids circular include with IScheduler.h

// One CPUWorker represents one CPU core (lecture §4 – processor data structure).
// The short-term scheduler hands ready processes to idle workers via assign().
//
// quantum=0      → run to completion (FCFS behaviour)
// quantum=N>0    → execute N instructions then yield back via scheduler.requeue() (RR)
// delaysPerExec  → extra ms sleep before each instruction (0 = no extra delay)
class CPUWorker {
public:
    CPUWorker(int id, IScheduler& scheduler,
              std::uint32_t quantum, std::uint32_t delaysPerExec);
    ~CPUWorker();

    void start(); // launch the worker thread
    void stop();  // signal shutdown and join

    // Short-term scheduler calls this to give the worker a ready process.
    void assign(std::shared_ptr<Process> p);

    bool                     isIdle()            const;
    int                      getId()             const;
    std::shared_ptr<Process> getCurrentProcess() const; // snapshot for Console

private:
    void workerLoop();

    int            id;
    IScheduler&    scheduler;
    std::uint32_t  quantum;       // 0 = non-preemptive
    std::uint32_t  delaysPerExec; // additional ms per instruction

    std::shared_ptr<Process>  currentProcess;
    std::atomic<bool>         running{false};
    std::atomic<bool>         idle{true};
    mutable std::mutex        mtx;
    std::condition_variable   cv;
    std::thread               workerThread;
};
