#include "RRScheduler.h"

// All bodies are stubs — implement the round-robin policy here. The FCFSScheduler.cpp
// in this same project is the reference for thread setup, the CV-driven dispatch loop,
// and clean shutdown; RR adds quantum accounting and tail-requeue on preemption.

RRScheduler::RRScheduler(int numCores, std::uint32_t quantumCycles)
    : numCores(numCores), quantumCycles(quantumCycles) {}

RRScheduler::~RRScheduler() {
    // TODO(student): ensure stop() has run so threads are joined.
}

void RRScheduler::addProcess(std::shared_ptr<Process> p) {
    // TODO(student): push onto readyQueue under queueMutex, then notify schedulerCv.
    (void)p;
}

void RRScheduler::start() {
    // TODO(student): set running=true, spawn worker threads, launch schedulerLoop().
}

void RRScheduler::stop() {
    // TODO(student): set running=false, wake all CVs, join scheduler + workers.
}

void RRScheduler::moveToFinished(std::shared_ptr<Process> p) {
    // TODO(student): append to finishedList under finishedMutex.
    (void)p;
}

void RRScheduler::notifyScheduler() {
    // TODO(student): notify schedulerCv so the dispatch loop re-evaluates.
}

std::vector<std::shared_ptr<Process>> RRScheduler::getRunningProcesses() const {
    return {}; // TODO(student)
}

std::vector<std::shared_ptr<Process>> RRScheduler::getFinishedProcesses() const {
    std::lock_guard<std::mutex> lock(finishedMutex);
    return finishedList;
}

int RRScheduler::getNumCores() const { return numCores; }

int RRScheduler::getActiveCores() const {
    return 0; // TODO(student): count cores currently running a process.
}

void RRScheduler::schedulerLoop() {
    // TODO(student): the round-robin dispatch loop:
    //   - dispatch the front of readyQueue to an idle core
    //   - let it run for up to quantumCycles ticks
    //   - on quantum expiry with work remaining, preempt and requeue at the tail
    //   - on completion, moveToFinished()
}
