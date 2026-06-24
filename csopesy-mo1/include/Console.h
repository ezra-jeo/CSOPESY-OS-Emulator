#pragma once
#include "FCFSScheduler.h"
#include "SystemConfig.h"
#include <string>
#include <vector>

// MO1 main-menu CLI running on the main thread.
//
// Recognised commands (see handlers below):
//   initialize | screen -s|-r|-ls | scheduler-start | scheduler-stop |
//   report-util | exit
//
// The seed only handled "screen -ls" and "exit"; the extra handlers are MO1 stubs for
// the implementer to fill in.
class Console {
public:
    explicit Console(FCFSScheduler& scheduler);

    // Blocks, reading stdin line by line, until the user types "exit".
    void run();

private:
    // Splits a raw input line into whitespace-separated tokens.
    static std::vector<std::string> tokenize(const std::string& line);

    // Dispatches one tokenized command. Returns false when the console should exit.
    bool dispatch(const std::vector<std::string>& args);

    // ── Command handlers (MO1) ────────────────────────────────────────────────
    void cmdInitialize();                                   // load + validate config.txt
    void cmdScreen(const std::vector<std::string>& args);   // -s <name> | -r <name> | -ls
    void cmdSchedulerStart();                               // begin batch generation
    void cmdSchedulerStop();                                // stop batch generation
    void cmdReportUtil();                                   // write csopesy-log.txt

    // ── screen sub-views ──────────────────────────────────────────────────────
    void screenSession(const std::string& name, bool resume); // attached sub-prompt loop
    void printProcessList() const;                          // screen -ls (seed: works)

    FCFSScheduler& scheduler;

    // Set true once `initialize` has loaded a valid config.txt. Gate every other
    // command behind this per the spec.
    bool         initialized = false;
    SystemConfig config;
};
