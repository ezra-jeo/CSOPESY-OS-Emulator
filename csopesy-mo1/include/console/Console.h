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

// System facade for the MO1 emulator. Owns the runtime state — config, scheduler, process
// generator, the process registry, and the batch-generation thread — and exposes the operations
// the UI screens invoke.
//
// The interactive UI is the `screen`-multiplexer model: ScreenManager runs the single input loop
// and delegates to the active Screen (MainMenuScreen for the menu, ProcessScreen for an attached
// process). Console::run() wires those up and owns startup/shutdown.
class Console {
public:
    Console();
    ~Console();

    // Boot, then hand control to the ScreenManager until the user exits.
    void run();

    // ── System operations (invoked by MainMenuScreen) ─────────────────────────
    void cmdInitialize();
    void cmdSchedulerStart();
    void cmdSchedulerStop();
    void cmdReportUtil();
    void printProcessList(std::ostream& os, bool color) const;

    bool isInitialized() const { return initialized; }

    // Process lookup/creation for `screen -r` / `screen -s`.
    std::shared_ptr<Process> findProcess(const std::string& name) const;
    std::shared_ptr<Process> getOrCreateProcess(const std::string& name);

private:
    bool         initialized = false;
    SystemConfig config;

    std::unique_ptr<IScheduler>       scheduler;
    std::unique_ptr<ProcessGenerator> generator;

    std::atomic<bool> generating{false};
    std::thread       genThread;

    mutable std::mutex                                        registryMutex;
    std::unordered_map<std::string, std::shared_ptr<Process>> registry;
};
