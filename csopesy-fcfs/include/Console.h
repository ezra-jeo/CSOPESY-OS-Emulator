#pragma once
#include "FCFSScheduler.h"
#include <string>

// Minimal CLI running on the main thread.
// Phase 1 recognises two commands: "screen -ls" and "exit".
class Console {
public:
    explicit Console(FCFSScheduler& scheduler);

    // Blocks, reading stdin line by line, until the user types "exit".
    void run();

private:
    void printProcessList() const;

    FCFSScheduler& scheduler;
};
