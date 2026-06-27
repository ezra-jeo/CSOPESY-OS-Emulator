# PPT Content — CSOPESY MO1

Slide-ready notes for the five required sections from the MO1 spec.
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
| `screen -s <name>` | create (or attach to existing) process, enter its screen |
| `screen -r <name>` | re-attach to an existing, non-finished process |
| `exit` | quit the emulator |

**Attached process screen (`ProcessScreen`) recognized commands:**
| Command | Action |
|---|---|
| `process-smi` | refresh the process detail + instruction view |
| `exit` | detach and return to main menu |

**Guard:** every post-init command (all except `initialize` / `exit`) is blocked with
`error: run initialize first` until `console.isInitialized()` returns true.

**Unknown input:** `csosh: command not found: <cmd>` (gray/yellow ANSI).

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
