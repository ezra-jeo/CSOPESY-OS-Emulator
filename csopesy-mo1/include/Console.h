#pragma once
#include "IScheduler.h"
#include "SystemConfig.h"
#include "ProcessGenerator.h"
#include "Process.h"
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <iosfwd>

// MO1 main-menu CLI running on the main thread.
//
// Recognised commands:
//   initialize | screen -s|-r|-ls | scheduler-start | scheduler-stop |
//   report-util | exit
//
// Console owns the scheduler (built at initialize time), the process generator,
// and the batch-generation thread. All post-init handlers are gated behind the
// `initialized` flag per the spec.
class Console {
public:
    Console();
    ~Console();

    // Blocks, reading stdin line by line, until the user types "exit".
    void run();

private:
    // Splits a raw input line into whitespace-separated tokens.
    static std::vector<std::string> tokenize(const std::string& line);

    // Dispatches one tokenized command. Returns false when the console should exit.
    bool dispatch(const std::vector<std::string>& args);

    // ── Command handlers ──────────────────────────────────────────────────────
    void cmdInitialize();
    void cmdScreen(const std::vector<std::string>& args);
    void cmdSchedulerStart();
    void cmdSchedulerStop();
    void cmdReportUtil();

    // ── screen sub-views ──────────────────────────────────────────────────────
    void screenSession(const std::string& name, bool resume);
    void printProcessList(std::ostream& os, bool color) const;

    // ── State ─────────────────────────────────────────────────────────────────
    bool         initialized = false;
    SystemConfig config;

    std::unique_ptr<IScheduler>       scheduler;
    std::unique_ptr<ProcessGenerator> generator;

    std::atomic<bool> generating{false};
    std::thread       genThread;

    mutable std::mutex                                        registryMutex;
    std::unordered_map<std::string, std::shared_ptr<Process>> registry;
};
