# PPT Content — CSOPESY MO1 / MO2

Slide-ready notes for the required sections from the MO1 spec (Sections 1-5), plus Section 6
added for MO2's demand-paging memory management requirement. This file accumulates across
iterations — MO1 sections are extended in place with MO2 additions rather than removed.
Each section maps to one PPT topic area; bullet points are slide talking-points.

---

## Section 1 — Command Recognition

**How user input reaches a handler**

- Single `std::getline(std::cin, line)` loop inside `ScreenManager::run()`.
- Tokenizer (`tokenize()`) splits on whitespace via `istringstream >> token`.
- Dispatched to the **active screen's** `handleCommand(args)` — whichever screen is on top of
  the navigation stack gets input focus.

**Main menu (`MainMenuScreen`) recognized commands:**
| Command | Action |
|---|---|
| `initialize` | load `config.txt`, build scheduler + generator |
| `scheduler-start` | begin batch process generation |
| `scheduler-stop` | stop batch process generation |
| `report-util` | write status snapshot to `csopesy-log.txt` |
| `screen -ls` | print Running / Sleeping / Finished table |
| `screen -s <name> [<size>]` | create (or attach to existing) process, optional explicit power-of-2 byte size, enter its screen |
| `screen -c <name> [<size>] "<ins>"` | create a process from a literal `;`-separated instruction string (DECLARE/ADD/SUBTRACT/WRITE/READ/PRINT, up to 50 instructions), enter its screen |
| `screen -r <name>` | re-attach to an existing, non-finished, non-violated process |
| `process-smi` (MO2) | system-wide CPU/memory utilization + per-running-process memory usage table |
| `vmstat` (MO2) | total/used/free memory, idle/active/total CPU ticks, num paged in/out |
| `exit` | quit the emulator |

**Attached process screen (`ProcessScreen`) recognized commands:**
| Command | Action |
|---|---|
| `process-smi` | refresh the process detail + instruction view (this is a *different* `process-smi` from the main-menu one above — same name, screen-scoped) |
| `exit` | detach and return to main menu |

**Guard:** every post-init command (all except `initialize` / `exit`) is blocked with
`error: run initialize first` until `console.isInitialized()` returns true.

**Unknown input:** `csosh: command not found: <cmd>` (gray/yellow ANSI).

**MO2 addition — `screen -r` on a memory-access-violation-terminated process:** checked *before*
the generic not-found path, so a violated process reports its violation instead of
`Process <name> not found.`:
```
Process <name> shut down due to memory access violation error that occurred at <HH:MM:SS>. <0xADDR> invalid.
```
`screen -s`/`screen -c` with an explicit size that fails the power-of-2-in-`[64, 65536]` check
print `invalid memory allocation` and create nothing (verified in testcases TC9/TC14).

---

## Section 2 — Console UI

**Boot sequence**
- `Console::run()` clears the screen and prints a box-draw ASCII logo ("CSOPESY") in cyan.
- Six animated boot messages follow (e.g. `[ OK ] Initializing kernel scheduler...`) with
  `sleep_for` delays to simulate a boot process.
- Prompt appears: `CSOPESY OS booted. Type 'initialize' to load config.txt.`

**ANSI color scheme** (constants used across all screens):
| Constant | Code | Use |
|---|---|---|
| `R` | `\033[0m` | Reset |
| `B` | `\033[1m` | Bold |
| `LG` | `\033[92m` | Bright green — success, running |
| `CY` | `\033[96m` | Bright cyan — labels, logo, current line |
| `WH` | `\033[97m` | Bright white — field names, attached prompt |
| `GR` | `\033[90m` | Gray — secondary info, hidden-line counts |
| `YL` | `\033[93m` | Yellow — warnings, sleeping, unknown cmds |

**ScreenManager navigation model**
- `registry`: named screens pre-registered (currently just `"main-menu"`).
- `stack`: top of stack has input focus; `Push` attaches a new screen, `Pop` returns.
- Single stdin loop — no nested loops, no duplicated REPL code.

**Main menu prompt:** `user@csopesy:~$` (bold green + cyan)

**Attached screen prompt:** `root:\>` (bold white)

**`screen -ls` output structure:**
```
  ╔════════════════════════════════════════╗
  ║   PROCESS SCHEDULER STATUS             ║
  ╚════════════════════════════════════════╝

  CPU Utilization :  75%
  Cores Used      : 3 / 4
  Cores Available : 1

  Running Processes:
  ----------------------------------------------------------------
  p01            (06/27/2026 02:15:30PM)  Core:0  58 / 1000
  ...

  Sleeping Processes:
  ----------------------------------------------------------------
  p03            (06/27/2026 02:15:31PM)  Sleeping  12 / 1000

  Finished Processes:
  ----------------------------------------------------------------
  p02            (06/27/2026 02:15:29PM)  Finished  500 / 500
```

**`process-smi` output structure:**
```
Process name: p01
ID: 1

Logs:
(06/27/2026 02:15:28PM) Core:0 "Hello world from p01!"
...

Current instruction line: 58
Lines of code: 1000

Instructions:
       ... 47 more above
  [58]: ADD(x, y, 4)       ← bold cyan, current line
   59 : DECLARE(a, 1000)
   60 : PRINT("Hello world from p01!")
       ... 940 more below
```

---

## Section 3 — Command Interpreter

**`ICommand` interface** (`include/commands/ICommand.h`)
- `virtual void execute(Process& proc)` — run the instruction, mutate the PCB.
- `virtual std::string toString() const` — source-text representation for the listing.
- `virtual uint32_t getInstructionCount() const { return 1; }` — always 1 for leaves.

**Six instruction types:**

### PRINT
- `execute()`: resolves the message (literal or `"prefix" + varName` form), then calls
  `proc.logMessage(coreId, message)` → appended to the in-memory `logs` vector.
- `toString()` → `PRINT("Hello world from p01!")`
- **Only command that writes to the Logs block** (per spec); all others write nothing.

### DECLARE
- `execute()`: `proc.symbolTable.setVariable(name, value)`.
- `toString()` → `DECLARE(x, 1000)`

### ADD
- `execute()`: `dest = clamp(op2.resolve(st) + op3.resolve(st), 0, 65535)`.
- Operands are `Operand` objects (literal uint16 or variable name); missing vars auto-declare to 0.
- `toString()` → `ADD(x, y, 4)`

### SUBTRACT
- `execute()`: `dest = clamp(op2.resolve(st) - op3.resolve(st), 0, 65535)` (floor at 0).
- `toString()` → `SUBTRACT(x, x, 1)`

### SLEEP
- `execute()`: `proc.setSleepRequest(N)` — sets a flag and tick count in the PCB.
  The executing `CPUWorker` detects the flag *after* `execute()` returns and yields the core.
  The watcher thread re-admits the process when `cpuTick ≥ wakeAt`.
- `toString()` → `SLEEP(3)`

### FOR (generation only — not a runtime instruction)
- `ProcessGenerator::makeFlat()` handles FOR by emitting `body × reps` flat leaf commands.
- **No `ForCommand` object is ever added to a process's `commandList`.**
- Each iteration of a FOR body is a separate top-level instruction: independently counted,
  independently logged, independently preemptible by the RR quantum.
- Nesting capped at depth 3 in `makeFlat()`; body length 1–3 commands; repeats 1–5.

**`Operand`** (`include/commands/Operand.h`)
- Either a literal `uint16_t` or a variable name.
- `resolve(SymbolTable&)`: returns literal value or looks up var (auto-declare to 0 if missing).
- `toString()`: `"42"` or `"x"`.

---

## Section 4 — Process Representation

**The PCB — `Process` class** (`include/process/Process.h`)

| Field | Type | Purpose |
|---|---|---|
| `pid` | `int` | Unique process ID (assigned at generation, 0-indexed) |
| `name` | `string` | Human name (`p01`, `p02`, … or custom from `screen -s`) |
| `state` | `enum` | `READY / RUNNING / WAITING / FINISHED` |
| `commandList` | `vector<shared_ptr<ICommand>>` | Flat instruction list (no FOR at runtime) |
| `commandCounter` | `int` | Index of *next* instruction (0-based internally; +1 for display) |
| `symbolTable` | `SymbolTable` | Per-process variable store (`name → int`) |
| `coreId` | `int` | Which core is executing this (-1 if none) |
| `sleepRequest` | `bool` | SleepCommand sets this; CPUWorker reads and clears it |
| `sleepTicks` | `uint8_t` | How many ticks to sleep |
| `startTime` | `time_t` | Wall time when first assigned to a core |
| `finishTime` | `time_t` | Wall time when last instruction completed |

**State machine:**
```
    READY ──(assigned to core)──> RUNNING
      ^                               │
      │ (quantum expired)             ├──(SLEEP)──> WAITING ──(wakeAt tick)──> READY
      │ (re-admit from sleep)         │
      └───────────────────────────────┴──(last instruction)──> FINISHED
```

**In-memory logging (PRINT-only):**
- `logs`: `vector<string>` — each entry is `"(timestamp) Core:N \"msg\""`.
- `logMessage(coreId, msg)`: formats and appends under a mutex.
- `getLogs()`: returns a mutex-guarded copy for safe reading by the console.
- Only `PrintCommand::execute()` calls `logMessage()`. Other commands leave `logs` unchanged.

**Instruction listing (for `process-smi`):**
- `getInstructionListing()` → `vector<string>` built by calling `toString()` on every entry
  in `commandList`. Used by `ProcessScreen::render()` for the windowed display.

**Process naming:**
- Auto-generated: `p` + zero-padded 2-digit pid (`p01, p02, …`).
- Custom: `screen -s <name>` calls `getOrCreateProcess(name)` which passes the name directly.

---

### MO2 addition — memory representation / addressing

**Every process owns its own virtual address space `[0, memSize)`.** `memSize` is fixed at
admission (`Process::bindMemory`), either from a `screen -s`/`screen -c` explicit size or a
rolled `[min-mem-per-proc, max-mem-per-proc]` value.

- **Symbol-table segment:** the first 64 bytes (`SymbolTable::SEGMENT_SIZE`) of every process's
  address space are reserved for its variables. Each variable gets a 2-byte-aligned offset
  (`SymbolTable::offsetFor`) — up to `SymbolTable::MAX_VARS = 32` slots. A 33rd distinct variable
  name is **silently ignored** (spec: "succeeding instructions involving variable declarations
  will be ignored"), not an error — verified in testcases TC12.
- **`memRead(addr, &out)` / `memWrite(addr, value)` are the only paths into a process's memory.**
  `DeclareCommand`/`AddCommand`/`SubtractCommand`/`PrintCommand`'s variable access and
  `ReadCommand`/`WriteCommand`'s direct address access all funnel through these two calls —
  there is no side channel that reads/writes process memory without going through the fault
  channel below.
- **Addresses in `READ`/`WRITE` (and the symbol table's internal offsets) are process-relative
  virtual addresses**, always in `[0, memSize)` — never physical/frame addresses. The same
  virtual address in two different processes maps to two entirely different physical frames (or
  neither may be resident at all).
- **Every `memRead`/`memWrite` can fault**, returning `false` and setting one of two outcomes on
  `Process::getFault()`:
  - `Violation` — address is `>= memSize` (out of bounds). Permanent: the process is terminated
    (`hasViolation()`, `getViolationTime()`, `getViolationAddr()` survive `clearFault()` for
    `screen -r` to report later).
  - `PageFault` — address is in-bounds but its page (`addr / pageSizeBytes`) isn't resident yet.
    Transient: cleared by `clearFault()` once the allocator installs the page.
- Every process is bound with `demandPaged=true`: pages start non-resident and are brought in on
  first fault, so both `Violation` and `PageFault` are live outcomes for every process.

---

## Section 5 — Scheduler Implementation

**Overview:** two schedulers (FCFS and RR) share a common base. Both run three helper threads
(scheduler dispatch, N workers, watcher/clock) on top of a ready queue.

---

### `IScheduler` interface (`include/scheduler/IScheduler.h`)
Key methods called by CPUWorker:
- `addProcess(proc)` — enqueue to ready queue.
- `requeue(proc)` — put preempted process at the tail (RR).
- `moveToFinished(proc)` — move to finished list.
- `addToWaiting(proc, wakeAt)` — park sleeping process on the waiting list.
- `notifyScheduler()` — wake the scheduler thread (core went idle).
- `getCpuTick()` → `uint64_t` — read the free-running tick counter.

---

### `SchedulerBase` (`include/scheduler/SchedulerBase.h`)
Shared implementation for tick counter, waiting list, and watcher thread.

**Free-running clock:**
```cpp
constexpr int CPU_CYCLE_MS = 10;   // 1 tick = 10 ms

void watcherLoop() {
    while (watcherRunning) {
        sleep_for(milliseconds(CPU_CYCLE_MS));
        cpuTick++;                 // free-running — advances even when queue is empty
        // move processes with wakeAt <= cpuTick from waiting list → ready queue
    }
}
```

Tick advances independently of instruction execution, so `SLEEP` timers and
`batch-process-freq` generation never deadlock when the ready queue drains.

---

### `CPUWorker` — clock-paced execution
Each of the N workers runs one thread. After being `assign()`ed a process:

```
for each instruction (up to quantum):
    target = getCpuTick() + 1 + delaysPerExec
    while getCpuTick() < target: sleep 1 ms    // pace to CPU_CYCLE_MS per instruction
    proc->executeCurrentCommand()
    proc->moveToNextLine()
    if proc->hasSleepRequest():
        addToWaiting(proc, getCpuTick() + sleepTicks)
        proc->setState(WAITING)
        break
```

After the loop: finished → `moveToFinished`; quantum expired → `requeue` (tail); sleep yield →
watcher handles re-admission. Worker sets `idle = true` and calls `notifyScheduler()`.

**MO2 addition — memory admission and page-fault handling** (brief recap; scheduling policy
itself is unchanged from MO1):
- Memory admission goes through `IMemoryAllocator::admit(proc, size)` on `MemoryManager`, the
  demand-paging memory manager.
- After `executeCurrentCommand()`, `CPUWorker` checks `proc->getFault()`. A `PageFault` calls
  `allocator.handleFault(*proc, proc->getFaultPage())`, then `proc->clearFault()`, then
  `continue`s the instruction loop **without** calling `moveToNextLine()` or incrementing
  `executed` — the faulted instruction restarts from the top next time it runs. This deliberately
  does **not** consume any of the process's RR quantum: fault resolution happens "indefinitely"
  while the process still holds the core, not as part of its fair instruction share.
- A `Violation` fault instead breaks the loop immediately and moves the process straight to
  `FINISHED` (see Section 6 for backing-store/eviction detail).

---

### `RRScheduler` — Round-Robin
- Quantum = `quantum-cycles` from config (e.g. 5 instructions per turn).
- Scheduler thread: waits on condition variable for (ready-queue non-empty AND idle core).
  Assigns front of queue to idle `CPUWorker.assign(proc)`.
- `requeue(proc)`: appends to the **tail** of the ready queue (fair round-robin rotation).
- Starts: N CPUWorkers + 1 scheduler thread + watcher.

---

### `FCFSScheduler` — First-Come-First-Served
- Same structure as RR but quantum = **0** (run to completion).
- CPUWorker loop condition `quantum > 0 && executed >= quantum` never fires → process runs
  until `isFinished()`. No `requeue()` calls.

---

### Batch process generation (`Console::cmdSchedulerStart`)
```
genThread:
    lastTick = getCpuTick()
    bootstrap = true
    while generating:
        now = getCpuTick()
        if bootstrap OR (now - lastTick >= batchProcessFreq):
            admit new process → registry + scheduler.addProcess()
            lastTick = now
            bootstrap = false
        sleep 1 ms  // poll loop — avoids busy-spin
```

`batchProcessFreq` is measured in CPU ticks (1 tick = 10 ms), so `batch-process-freq 1`
admits roughly one process per 10 ms; `batch-process-freq 100` ≈ one per second.

---

### Summary table

| Config key | Effect |
|---|---|
| `num-cpu` | Number of CPUWorker threads |
| `scheduler rr` / `fcfs` | Which policy; quantum = quantum-cycles or 0 |
| `quantum-cycles` | RR time slice (instructions per turn) |
| `batch-process-freq` | Ticks between process admissions |
| `min-ins` / `max-ins` | Instruction count range per process |
| `delays-per-exec` | Extra ticks waited before each instruction (0 = minimum pace) |
| `min-mem-per-proc` / `max-mem-per-proc` (MO2) | Range a `scheduler-start`/no-size `screen -s`/`-c` process's memory footprint is rolled from |

---

## Section 6 — Memory Management: Demand Paging and Backing Store

**One allocator behind one interface (`IMemoryAllocator`, `include/memory/IMemoryAllocator.h`),
implemented by `MemoryManager` (`include/memory/MemoryManager.h`) — the rest of the program only
ever talks to `IMemoryAllocator`:**
- Fixed frame table (`maxOverallMem / memPerFrame` frames), no contiguity requirement.
- `admit()` never fails — no physical frames are needed up front.
- Pages start non-resident; brought into a frame on first fault.
- Eviction is FIFO over `loadOrder` (a `deque<frame index>`).

**`MemoryManager` (`include/memory/MemoryManager.h` / `src/memory/MemoryManager.cpp`)**
- Physical memory = `frames.size() = maxOverallMem / memPerFrame` fixed-size `Frame` slots.
- `admit(proc, size)`: no frame allocation — just `proc.setMemory(0, size)` +
  `proc.bindMemory(size, frameBytes, /*demandPaged=*/true)`. Every access starts as a fault.
- `handleFault(proc, vpage)`:
  1. Already resident → `return true` (race-safe re-check under the lock).
  2. Free frame available → use it.
  3. Otherwise: pop the **FIFO victim** (`loadOrder.front()`), `extractPageBytes()` its current
     owner's page, stash the bytes in the in-memory `store` map (`(ownerName, page) → bytes`),
     `invalidatePage()` the victim, count a **page-out**.
  4. Load the incoming page's bytes — either from `store` (was paged out before) or zero-filled
     (first touch) — via `installPageBytes()`, mark the frame occupied, push it onto the back of
     `loadOrder` (newest), count a **page-in**.
  5. Rewrite `csopesy-backing-store.txt` from the current `store` map (see below).
- `pagedIn()` / `pagedOut()` — atomic counters surfaced by `vmstat`.

**Backing store (`csopesy-backing-store.txt`):**
- Created empty (`entries: 0`) the moment `MemoryManager` is constructed at `initialize` —
  present for the whole run, not just after the first fault.
- Rewritten on every eviction: human-readable dump of `owner=<name> page=<n> bytes=<frameBytes>`
  blocks, each page's bytes shown as little-endian `uint16` words at `[offset]`.
- On a later `admit`/eviction/page-in for the same `(owner, page)` key, the stashed entry is
  read back and removed from `store` — i.e. it round-trips exactly like a real backing store,
  confirmed by testcase TC11 (content changes across snapshots taken seconds apart while the
  scheduler runs).

**Page-fault-and-restart flow (ties Section 5 together):** `CPUWorker` executes an instruction →
`Process::memRead`/`memWrite` finds the page not resident → sets `MemFault::PageFault` → worker
calls `allocator.handleFault()` → worker `clearFault()`s and restarts the *same* instruction
(program counter not advanced) → on the retry the page is resident so the instruction actually
completes and the counter advances normally.

**`process-smi` / `vmstat` as the debugging surface for all of this** (`Console::cmdProcessSmi` /
`Console::cmdVmstat`, `src/console/Console.cpp`):
- `process-smi`: system CPU utilization, `memory->usedBytes()`/`totalBytes()` and derived percent,
  then each **running** process's name + `getMemSize()` (total admitted address-space size, not
  live resident-byte count — the allocator has no per-process resident-byte query).
- `vmstat`: `totalBytes` / `usedBytes` / `freeBytes`, cumulative idle/active/total CPU ticks
  (`SchedulerBase`), and `pagedIn()` / `pagedOut()` — the two counters used in TC10/TC11 to prove
  paging activity is real and ongoing under memory pressure.
