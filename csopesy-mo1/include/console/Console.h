#pragma once
#include "IScheduler.h"
#include "IMemoryAllocator.h"
#include "MemoryManager.h"
#include "PagingAllocator.h"
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
    void cmdProcessSmi() const;   // main-menu-level summary (NOT ProcessScreen's per-process one)
    void cmdVmstat() const;

    bool isInitialized() const { return initialized; }

    // Process lookup/creation for `screen -r` / `screen -s` / `screen -c`.
    std::shared_ptr<Process> findProcess(const std::string& name) const;

    // screen -s <name> <size> (size may be 0 meaning "not given" — rolls one from
    // [config.minMemPerProc, config.maxMemPerProc] as a power of two, same helper as below).
    // Existing `name`: attaches to it unchanged, `size` is ignored (matches MO1's original
    // attach-to-existing behavior — screen -s doubles as "reattach").
    // New `name` with an invalid explicit `size` (not a power of two, or outside [64, 65536]):
    // returns nullptr, err = "invalid memory allocation".
    // New `name`, valid size: generates the process via the existing random-instruction path
    // (ProcessGenerator::generate(name)), records the size via setRequestedMemSize, registers it,
    // queues it on the scheduler exactly like the old getOrCreateProcess did.
    std::shared_ptr<Process> createProcess(const std::string& name, std::uint64_t size, std::string& err);

    // screen -c <name> [<size>] "<instructions>". `size == 0` means "not given" — rolled the same
    // way. Fails (nullptr + err) if: name already exists (err = "Process <name> already exists."),
    // size invalid (err = "invalid memory allocation"), or instrText fails ProcessGenerator's
    // parser (err = "invalid command", propagated as-is). On success: uses
    // ProcessGenerator::createEmpty + buildFromInstructionText, records the size, registers,
    // queues — same as createProcess.
    std::shared_ptr<Process> createProcessWithInstructions(const std::string& name, std::uint64_t size,
                                                             const std::string& instrText, std::string& err);

private:
    // Returns true + *out set if `size` is 0 (roll a power-of-two from [config.minMemPerProc,
    // config.maxMemPerProc]) or already a valid power of two in [64, 65536]. Returns false if
    // `size` is nonzero but invalid.
    bool resolveMemSize(std::uint64_t size, std::uint64_t& out) const;
    bool         initialized = false;
    SystemConfig config;

    std::unique_ptr<IMemoryAllocator> memory;
    std::unique_ptr<IScheduler>       scheduler;
    std::unique_ptr<ProcessGenerator> generator;

    std::atomic<bool> generating{false};
    std::thread       genThread;

    mutable std::mutex                                        registryMutex;
    std::unordered_map<std::string, std::shared_ptr<Process>> registry;
};
