# CSOPESY MO1 — Current State & Architecture

A living reference for the `csopesy-mo1` process scheduler + CLI. For each component it
explains **what it is technically** and **what it currently does / has today**.

Branch: `fix/timing-and-for` → merged into `feat/cpu-emulator-cli` + `test/cpu-emulator-cli`.
Last updated after the ScreenManager refactor, component subdirectory layout, Config.h removal,
clock-paced execution, FOR loop flattening, and windowed process-smi.

---

## 1. What this is

A console OS emulator: a command-line interpreter that drives a multi-core CPU **process
scheduler**. You type commands; the scheduler runs dummy processes (sequences of instructions)
across N simulated cores using FCFS or Round-Robin, and you can attach to any process to watch it
execute.

### Build & run
```bash
cd csopesy-mo1
cmake -S . -B build
cmake --build build --parallel
./build/csopesy
```
Reads `config.txt` on `initialize`. C++20, standard library only (threads, no external deps).

---

## 2. Big picture — how a run flows

```
main() ──> Console::run()
              │  prints boot banner + [  OK  ] messages
              │
              └─> ScreenManager::run("main-menu")     [single stdin loop]
                      │
                      ├── MainMenuScreen::handleCommand()  ← input focus (main menu)
                      │     initialize      → Console::cmdInitialize()
                      │     scheduler-start → Console::cmdSchedulerStart()
                      │     scheduler-stop  → Console::cmdSchedulerStop()
                      │     report-util     → Console::cmdReportUtil()
                      │     screen -ls      → Console::printProcessList()
                      │     screen -s/-r    → push ProcessScreen onto stack
                      │     exit            → ScreenAction::quit → loop ends
                      │
                      └── ProcessScreen::handleCommand()   ← input focus (attached)
                            process-smi → render() (windowed instruction listing)
                            exit        → ScreenAction::pop → back to main menu
```

`Console` is a **system facade** — it owns config/scheduler/generator/registry/genThread
and exposes operations. `ScreenManager` owns the stdin loop and the screen stack.

### Scheduler internals
```
IScheduler (RR or FCFS)
  • ready queue
  • SCHEDULER thread    — dispatch front-of-queue → idle CPUWorker
  • N CPUWorker threads — execute instructions, clock-paced
  • WATCHER/CLOCK thread — free-running tick + re-admit sleeping processes
  • finished list
```

### Threads alive during a run
| Thread | Count | Job |
|---|---|---|
| Console REPL | 1 (main) | ScreenManager stdin loop: render and dispatch commands |
| Generation | 1 (while scheduler-start) | Admit one process every `batch-process-freq` ticks |
| Scheduler | 1 | Dispatch ready processes to idle CPUWorkers |
| CPUWorker | `num-cpu` | Execute a process's instructions (clock-paced) |
| Watcher / clock | 1 | Advance CPU tick (10 ms/tick) + wake sleeping processes |

---

## 3. File layout

```
csopesy-mo1/
├── include/
│   ├── commands/   ICommand.h  AddCommand.h  DeclareCommand.h  ForCommand.h
│   │               Operand.h   PrintCommand.h  SleepCommand.h  SubtractCommand.h
│   ├── console/    Screen.h  ScreenManager.h  Console.h
│   │               MainMenuScreen.h  ProcessScreen.h
│   ├── process/    Process.h  CPUWorker.h  ProcessGenerator.h  SymbolTable.h
│   ├── scheduler/  IScheduler.h  SchedulerBase.h
│   │               FCFSScheduler.h  RRScheduler.h
│   └── config/     SystemConfig.h
└── src/            (mirrors include/ layout)
    ├── commands/   *.cpp
    ├── console/    Console.cpp  MainMenuScreen.cpp  ProcessScreen.cpp  ScreenManager.cpp
    ├── process/    CPUWorker.cpp  Process.cpp  ProcessGenerator.cpp
    ├── scheduler/  FCFSScheduler.cpp  RRScheduler.cpp  SchedulerBase.cpp
    ├── config/     SystemConfig.cpp
    └── main.cpp
```

No `Config.h`. No `EXEC_DELAY_MS`. Runtime timing is entirely clock-paced (see SchedulerBase).

---

## 4. Components

### `main.cpp`
4 lines — constructs `Console` and calls `run()`.

---

### `Console` (`include/console/Console.h` / `src/console/Console.cpp`) — system facade
**Technical:** owns the scheduler, process generator, process registry (`name → shared_ptr<Process>`,
mutex-guarded), and the generation thread. Exposes system operations; no longer contains a REPL.

**Public API:**
- `run()` — prints boot banner then delegates to `ScreenManager`.
- `cmdInitialize()` — parses `config.txt`, builds scheduler + generator, starts scheduler.
- `cmdSchedulerStart()` / `cmdSchedulerStop()` — start/stop the generation thread.
- `cmdReportUtil()` — writes `printProcessList()` output to `csopesy-log.txt`.
- `printProcessList(ostream&, bool color)` — Running / Sleeping / Finished table.
- `isInitialized()` — gate used by MainMenuScreen.
- `findProcess(name)` — registry lookup; returns `nullptr` if not found.
- `getOrCreateProcess(name)` — registry lookup; creates + admits if missing.

---

### `ScreenManager` (`include/console/ScreenManager.h` / `src/console/ScreenManager.cpp`)
**Technical:** owns the single `std::getline` stdin loop plus two structures:
- **registry** — named screens registered before `run()` (currently only `"main-menu"`).
- **stack** — active navigation; the top of the stack has input focus.

`run(initialName)` pushes the named screen and loops: reads a line, tokenizes it, calls
`stack.top()->handleCommand(args)`, then acts on the returned `ScreenAction`:
- `Stay` → keep looping.
- `Push(screen)` → call `screen->onEnter()`, push onto stack.
- `Pop` → pop, call `onEnter()` on the new top (if any).
- `Quit` → clear the stack and return.

---

### `Screen` interface + `ScreenAction` (`include/console/Screen.h`)
`Screen`: pure virtual `name()`, `prompt()`, `onEnter()`, `handleCommand(args)`.
`ScreenAction`: tagged union `{Stay, Push, Pop, Quit}` with the next screen for Push.
Static factory methods: `stay()`, `push(screen)`, `pop()`, `quit()`.

---

### `MainMenuScreen` (`include/console/MainMenuScreen.h` / `src/console/MainMenuScreen.cpp`)
**Technical:** implements `Screen`; handles the top-level command set.
- Prompt: `user@csopesy:~$` (bold green/cyan ANSI).
- Recognized before init: `initialize`, `exit`.
- Recognized after init: `screen`, `scheduler-start`, `scheduler-stop`, `report-util`.
- Anything else → `csosh: command not found: <cmd>`.
- `screen -s <name>` → `ScreenAction::push(ProcessScreen(getOrCreateProcess(name)))`.
- `screen -r <name>` → `ScreenAction::push(ProcessScreen(findProcess(name)))` if the process
  exists and is not finished; otherwise prints `Process <name> not found.`.

---

### `ProcessScreen` (`include/console/ProcessScreen.h` / `src/console/ProcessScreen.cpp`)
**Technical:** implements `Screen`; the view for an attached process.
- Prompt: `root:\> ` (bold white ANSI).
- `onEnter()` calls `render()` immediately on attach.
- `process-smi` → `render()` again; `exit` → `ScreenAction::pop()`.

**`render()` — windowed instruction listing:**
Shows the process's name, PID, Logs (PRINT-only), and status (`Current instruction line` /
`Lines of code` or `Finished!`), then a windowed slice of the instruction listing:

```
       ... 47 more above
  [58]: ADD(x, y, 4)           ← current line (bold cyan [N])
   59 : DECLARE(a, 1000)
   ...
       ... 942 more below
```

Window logic (`CONTEXT = 10`): `lo = max(1, cur-10)`, `hi = min(total, cur+10)`.
If `cur ≤ 0` (not started): top `2*CONTEXT+1` lines. Hidden lines summarized in gray.
Small processes (`total ≤ 21`) show the full listing with no summary lines.

---

### `SystemConfig` (`include/config/SystemConfig.h` / `src/config/SystemConfig.cpp`)
**Technical:** parses `config.txt` key–value pairs and validates ranges.

All 7 parameters:
| Key | Type | Range |
|---|---|---|
| `num-cpu` | int | [1, 128] |
| `scheduler` | enum | `fcfs` / `rr` (quotes stripped — `"rr"` works) |
| `quantum-cycles` | uint32 | ≥ 1 |
| `batch-process-freq` | uint32 | ≥ 1 |
| `min-ins` | uint32 | ≥ 1 |
| `max-ins` | uint32 | ≥ min-ins |
| `delays-per-exec` | uint32 | ≥ 0 (alias `delay-per-exec` also accepted) |

Unknown keys and out-of-range values produce a human-readable error returned by `load()`.

---

### `ProcessGenerator` (`include/process/ProcessGenerator.h` / `src/process/ProcessGenerator.cpp`)
**Technical:** builds dummy processes. Names them `p01, p02, …` (sequential from `nextPid`).

Key design — **FOR loops are flattened at generation** (not stored as ForCommand objects):
- `makeFlat(pid, name, rng, depth)` returns `vector<shared_ptr<ICommand>>`.
  - Leaf types (PRINT/DECLARE/ADD/SUBTRACT/SLEEP) return a single-element vector.
  - FOR case: generates a body of 1–3 flat commands (recursive, depth ≤ 3), then emits
    `body × reps` (1–5) as a flat vector — no ForCommand created.
- `buildInstructions(proc)` draws a count from `[min-ins, max-ins]`, then calls `makeFlat()`
  in a loop, adding each resulting leaf individually until the count is reached.

Result: every instruction in a process's `commandList` is a leaf (`ICommand` returning
`getInstructionCount() == 1`). FOR iterations are separately counted, separately logged,
and separately preemptible.

---

### `Process` / PCB (`include/process/Process.h` / `src/process/Process.cpp`)
**Technical:** the Process Control Block.

Fields:
- `pid`, `name` (immutable after construction)
- `state`: `READY | RUNNING | WAITING | FINISHED`
- `commandList`: `vector<shared_ptr<ICommand>>` (flat; no FOR nesting at runtime)
- `commandCounter`: index of next instruction to execute (1-based for display)
- `SymbolTable`: variable store
- `coreId`: which CPUWorker is running this (-1 if none)
- `sleepRequest` flag + `sleepTicks` count (set by SleepCommand)
- `startTime`, `finishTime` (`time_t`)

Logging (PRINT-only, per spec):
- `log(string)` / `getLogs()` / `logMessage(coreId, msg)` — mutex-guarded in-memory `logs` vector.
- Only `PrintCommand::execute()` calls `logMessage()`; other commands write nothing to logs.

`getInstructionListing()` returns `vector<string>` via `toString()` on each command (used by
`ProcessScreen::render()` for the windowed display).

---

### `SymbolTable` (`include/process/SymbolTable.h`)
`name → int` map. `getVariable()` returns 0 for unknown names; `hasVariable()` for the
auto-declare check. Used by ADD/SUBTRACT/DECLARE/PRINT.

---

### `Operand` (`include/commands/Operand.h` / `src/commands/Operand.cpp`)
An ADD/SUBTRACT argument — either a literal `uint16` or a variable name.
`resolve(SymbolTable&)` reads the value (auto-declaring missing vars as 0, clamping to [0,65535]).
`toString()` renders as a number or variable name for the instruction listing.

---

### `ICommand` + the six command types (`include/commands/`)
`ICommand` base: pure virtual `execute(Process&)` and `toString()`;
default `getInstructionCount() → 1`.

| Command | `execute()` | `toString()` |
|---|---|---|
| **PRINT** | Appends `(timestamp) Core:N "msg"` to `proc.logs` via `logMessage()`. Resolves variable-form messages at execution time. | `PRINT("Hello world from p01!")` |
| **DECLARE** | `proc.symbolTable.setVariable(name, value)` | `DECLARE(x, 1000)` |
| **ADD** | `dest = clamp(op2.resolve() + op3.resolve(), 0, 65535)` | `ADD(x, y, 4)` |
| **SUBTRACT** | `dest = clamp(op2.resolve() - op3.resolve(), 0, 65535)` | `SUBTRACT(x, x, 1)` |
| **SLEEP** | `proc.setSleepRequest(N)` (sets a flag; CPUWorker yields the core) | `SLEEP(3)` |
| **FOR** | *(not instantiated at runtime — flattened by `makeFlat()` at generation)* | *(N/A)* |

Only PRINT writes to the Logs block; all other commands are visible only in the Instructions
listing (via `toString()`), keeping Logs PRINT-only per spec.

---

### `IScheduler` (`include/scheduler/IScheduler.h`)
Interface shared by workers and the console: `addProcess`, `requeue`, `start`, `stop`,
`moveToFinished`, `notifyScheduler`, waiting-list + tick methods, `getRunningProcesses`,
`getFinishedProcesses`, `getNumCores`, `getActiveCores`. Lets `CPUWorker` call back without
knowing the scheduling policy.

---

### `SchedulerBase` (`include/scheduler/SchedulerBase.h` / `src/scheduler/SchedulerBase.cpp`)
Shared base for both policies. Owns the **CPU tick counter**, the **waiting list**, and the
**watcher thread**.

```cpp
namespace { constexpr int CPU_CYCLE_MS = 10; }

void SchedulerBase::watcherLoop() {
    while (watcherRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(CPU_CYCLE_MS));
        incrementTick();     // free-running CPU clock
        // re-admit sleepers whose wake tick has passed → requeueReady(proc)
    }
}
```

The watcher is the **only** thing that advances the tick. Because the tick keeps moving even when
every process is sleeping or the ready queue is empty, `SLEEP` timers and `batch-process-freq`
generation can never deadlock.

---

### `RRScheduler` (`include/scheduler/RRScheduler.h` / `src/scheduler/RRScheduler.cpp`)
Round-Robin. Scheduler thread waits for (work + idle core) → assigns front of ready queue to an
idle `CPUWorker` with `quantum = quantum-cycles`. Preempted processes are requeued at the
**tail** via `requeue()`. Starts N workers + scheduler loop + watcher; clean stop/join.

---

### `FCFSScheduler` (`include/scheduler/FCFSScheduler.h` / `src/scheduler/FCFSScheduler.cpp`)
Identical structure to RR but workers receive `quantum = 0` → each process runs **to completion**
before the core takes another (no preemption).

---

### `CPUWorker` (`include/process/CPUWorker.h` / `src/process/CPUWorker.cpp`)
One thread per core. Waits idle until `assign(proc)` is called, then:

1. Binds the core to the PCB (`setCoreId(id)`, `setState(RUNNING)`).
2. Executes up to `quantum` instructions (0 = to completion):
   - **Clock-paced:** before each instruction, busy-waits until
     `getCpuTick() ≥ current + 1 + delaysPerExec`. This paces execution to `CPU_CYCLE_MS`
     wall-clock time per instruction when `delays-per-exec = 0`.
   - Calls `proc->executeCurrentCommand()` + `moveToNextLine()`.
   - **SLEEP detect:** if `proc->hasSleepRequest()`, computes `wakeAt = tick + sleepTicks`,
     calls `scheduler.addToWaiting(proc, wakeAt)`, sets state `WAITING`, and breaks.
3. Dispatches result:
   - Yielded for sleep → watcher re-admits via `requeueReady()`.
   - Finished → `scheduler.moveToFinished(proc)`.
   - Quantum expired → `scheduler.requeue(proc)` (tail of ready queue, RR preemption).
4. Clears slot, sets `idle = true`, notifies scheduler.

---

## 5. Configuration (`config.txt`)
```
num-cpu 4            # cores [1,128]
scheduler rr         # rr | fcfs  (quotes accepted: scheduler "rr")
quantum-cycles 5     # RR time slice (instructions per turn)
batch-process-freq 1 # admit one process every N CPU ticks
min-ins 1000         # min instructions per process
max-ins 2000         # max instructions per process
delays-per-exec 0    # extra CPU ticks to wait before each instruction
                     # (also accepted: delay-per-exec)
```

---

## 6. Current status — requirements checklist (all verified)

| Requirement | Status |
|---|---|
| Commands PRINT / DECLARE / ADD / SUBTRACT / SLEEP / FOR | ✅ all functional |
| PRINT default `"Hello world from <name>!"` + variable form | ✅ |
| uint16 clamp [0,65535] & auto-declare missing vars to 0 | ✅ |
| FOR nesting ≤ 3 at generation; each iteration a separate top-level instruction | ✅ |
| FOR iterations separately counted, logged, and preemptible | ✅ (flattened by makeFlat) |
| Processes run to completion (`Finished n/n`) | ✅ |
| `process-smi`: name, ID, Logs (PRINT-only), instruction line, `Finished!` | ✅ |
| Windowed process-smi (±10 lines, `... N more above/below`) | ✅ |
| `screen -r` missing/finished → `Process … not found.` | ✅ |
| `screen -ls` / `report-util` → `csopesy-log.txt` | ✅ |
| `screen -ls` shows Running / Sleeping / Finished (state-driven) | ✅ |
| All 7 config params parsed + validated | ✅ |
| `delay-per-exec` accepted as alias for `delays-per-exec` | ✅ |
| Quoted scheduler value (`scheduler "rr"`) accepted | ✅ |
| `batch-process-freq` measured in CPU ticks (tick-faithful) | ✅ |
| Clock-paced execution (1 + delays-per-exec ticks per instruction) | ✅ |
| FCFS + RR both schedule; watcher is the free-running clock | ✅ |
| Component subdirectory layout (commands/console/process/scheduler/config) | ✅ |
| No Config.h / no EXEC_DELAY_MS; timing is config-driven | ✅ |

---

## 7. Recent changes

1. **ScreenManager + Screen multiplexer** — `Console.run()` now delegates to `ScreenManager`;
   `MainMenuScreen` and `ProcessScreen` implement the `Screen` interface. Two duplicated REPL
   loops replaced by a single `ScreenManager::run()` loop with a push/pop stack.
2. **Console → facade** — all UI logic moved to MainMenuScreen; `Console` exposes only system
   operations (`cmdInitialize`, `cmdSchedulerStart`, etc.).
3. **Component subdirectory layout** — files split into `include/commands`, `include/console`,
   `include/process`, `include/scheduler`, `include/config` (and matching `src/` subdirs).
   CMakeLists.txt updated with `-iquote` per-dir include paths.
4. **Config.h removed** — `EXEC_DELAY_MS` gone. Observable timing now comes from the
   clock-paced CPU cycle (`CPU_CYCLE_MS = 10` ms/tick) and `delays-per-exec` in config.
5. **Clock-paced execution** — watcher sleeps 10 ms per tick; each instruction waits
   `(1 + delays-per-exec)` ticks before executing. At `delays-per-exec 0`, a 1000-instruction
   process takes ~10 s, keeping it observable.
6. **FOR loop flattening** (`makeFlat()` in ProcessGenerator) — no ForCommand objects at runtime;
   each body-×-repetitions result is stored as individual top-level instructions. Generator
   infinite loop fixed (`ctr` incremented per flat leaf).
7. **Windowed process-smi** (ProcessScreen::render, `CONTEXT = 10`) — shows ±10 lines around
   the current instruction instead of the full 1000–2000 line listing.
8. **Config aliases** — `delay-per-exec` accepted as alias for `delays-per-exec`; quoted
   scheduler values (`scheduler "rr"`) work.

---

## 8. Decisions & caveats

- **CPU tick model:** 1 tick = 10 ms wall-clock (`CPU_CYCLE_MS`). `SLEEP(N)` ≈ N×10 ms.
  `batch-process-freq N` ≈ one process per N×10 ms. This matches the spec's free-running
  counter pseudocode; it is *not* "one tick per executed instruction."
- **Sleeping visibility:** at `delays-per-exec 0`, SLEEP lasts `ticks × 10 ms`. The generator
  emits 1–5 tick SLEEPs → 10–50 ms. A `screen -ls` snapshot will rarely catch a sleeper.
  Increase SLEEP values or slow the clock to make the Sleeping section observable.
- **No ForCommand at runtime:** `ForCommand.cpp` and `.h` are still compiled but
  `ProcessGenerator` never calls its constructor. The class exists only for the shape.
- **`screen -r` on finished:** prints `Process <name> not found.` (same as missing). The spec
  says "not found" — distinguishing finished vs. missing is not required.
- **Logs grow unbounded:** every PRINT appends to the in-memory `logs` vector for the life of
  the process. No tail-limit is applied (out of scope for current requirements).

---

## 9. MO2 — Demand Paging Additions

Everything below is new relative to Sections 1-8; nothing above changed behaviourally except
where noted. `config.txt`'s presence of `min-mem-per-proc`/`max-mem-per-proc` is the single
switch that turns all of this on (`SystemConfig::sawMinMaxMemPerProc`,
`include/config/SystemConfig.h` / `src/config/SystemConfig.cpp`).

### `IMemoryAllocator` seam (`include/memory/IMemoryAllocator.h`)
Abstract interface `Console`/`SchedulerBase` hold instead of a concrete allocator type:
`allocate`/`deallocate` (legacy), `admit(Process&, size)`, `handleFault(Process&, vpage)`,
`isDemandPaged()`, `usedBytes`/`freeBytes`/`totalBytes`, `pagedIn`/`pagedOut`.
- **`MemoryManager`** (`include/memory/MemoryManager.h` / `src/memory/MemoryManager.cpp`) — a thin
  facade, also implementing `IMemoryAllocator`. Its constructor
  (`demandPaged, totalBytes, frameBytes`) picks one of the two strategies below and owns it as
  `std::unique_ptr<IMemoryAllocator> strategy`; every method is a one-line forward to `strategy->`.
  `Console::cmdInitialize` (`src/console/Console.cpp`) always constructs a `MemoryManager` —
  `std::make_unique<MemoryManager>(sawMinMaxMemPerProc, maxOverallMem, memPerFrame)` — so the rest
  of the program never needs to know or care which strategy is underneath.
- **`FlatMemoryAllocator`** (`include/memory/FlatMemoryAllocator.h` /
  `src/memory/FlatMemoryAllocator.cpp`) — the pre-existing MO1 flat first-fit allocator, unchanged
  logic, now one of `MemoryManager`'s two strategies. `admit()` binds every page resident
  immediately (`Process::bindMemory(..., demandPaged=false)`); its `handleFault()` is a
  never-really-reached `return true`. `pagedIn()`/`pagedOut()` always 0.
- **`PagingAllocator`** (`include/memory/PagingAllocator.h` / `src/memory/PagingAllocator.cpp`) —
  the other strategy. Fixed frame table (`maxOverallMem / memPerFrame` frames), FIFO eviction
  (`loadOrder` deque), an in-memory `store` map mirrored to `csopesy-backing-store.txt` on every
  eviction.

### `Process` virtual-address-space / fault-channel API (`include/process/Process.h` /
`src/process/Process.cpp`)
- `bindMemory(memSize, pageSize, demandPaged)` sets up `[0, memSize)`; `isPaged()`,
  `getPageCount()`, `getPageSizeBytes()`.
- `memRead(addr, &out)` / `memWrite(addr, value)` — the *only* paths into process memory. Return
  `false` + set `getFault()` (`PageFault` or `Violation`) on failure.
- `declareVar`/`readVar`/`writeVar` — offset-backed via `SymbolTable::offsetFor`/`hasOffset`
  (`include/process/SymbolTable.h`); the first 64 bytes of every process's address space are the
  32-slot symbol-table segment. A 33rd distinct variable name silently no-ops (spec-mandated,
  not an error) instead of writing anywhere.
- `isPageResident`/`extractPageBytes`/`installPageBytes`/`invalidatePage` — the paging
  allocator's only touchpoints into a process's page table; the flat allocator never calls these.
- Permanent violation record: `hasViolation()`/`getViolationTime()`/`getViolationAddr()` survive
  `clearFault()`, read later by `MainMenuScreen::handleScreen`'s `screen -r` violation check
  (`src/console/MainMenuScreen.cpp`).

### `CPUWorker` fault-restart loop (`src/process/CPUWorker.cpp`)
After `executeCurrentCommand()`: a `Violation` breaks the loop and the process moves straight to
`FINISHED`. A `PageFault` calls `allocator.handleFault(*proc, proc->getFaultPage())`,
`clearFault()`s, then `continue`s — **without** `moveToNextLine()` or incrementing `executed` —
so the faulted instruction restarts and, crucially, fault resolution never consumes any of the
process's RR quantum.

### `ReadCommand` / `WriteCommand` (`include/commands/{Read,Write}Command.h` /
`src/commands/{Read,Write}Command.cpp`)
`READ var 0xADDR` → `memRead` then `declareVar`. `WRITE 0xADDR value-or-var` → resolve the
operand first (so a variable-valued RHS that itself faults is restart-safe), then `memWrite`.
Both are no-ops on failure — they rely entirely on the CPUWorker fault-restart loop above, never
retry internally.

### `screen -c` instruction-text parser (`ProcessGenerator::buildFromInstructionText`,
`src/process/ProcessGenerator.cpp`)
Parses a `;`-separated instruction string (quote-aware split so `;` inside a `PRINT("...")`
literal doesn't break the split) into `DECLARE`/`ADD`/`SUBTRACT`/`WRITE`/`READ`/`PRINT` commands.
Caps at 50 instructions; any parse failure returns `"invalid command"` via the `err` out-param.
`MainMenuScreen::handleScreen`'s `screen -c` branch (`src/console/MainMenuScreen.cpp`) is the
only caller.

### `process-smi` / `vmstat` (`Console::cmdProcessSmi` / `Console::cmdVmstat`,
`src/console/Console.cpp`)
Main-menu commands (distinct from `ProcessScreen`'s attached-screen `process-smi`, which just
re-renders the current process). `process-smi`: system CPU util + `memory->usedBytes()`/
`totalBytes()` + a per-running-process `getMemSize()` table. `vmstat`: total/used/free memory,
cumulative idle/active/total CPU ticks (`IScheduler::getIdleTicks`/`getActiveTicks`/
`getTotalTicks`), and `memory->pagedIn()`/`pagedOut()`.
