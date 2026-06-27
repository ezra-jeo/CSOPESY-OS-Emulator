# CSOPESY MO1 — Current State & Architecture

A living reference for the `csopesy-mo1` process scheduler + CLI. For each component it
explains **what it is technically** and **what it currently does / has today**.

Branch: `feat/cpu-emulator-cli`. Last updated after the PRINT-output, tick-faithful clock, and
process-smi instruction-listing work.

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
main() ──> Console.run()  [main thread: the REPL]
              │
              │ initialize          → SystemConfig.load("config.txt")
              │                       builds RRScheduler or FCFSScheduler + ProcessGenerator
              │
              │ scheduler-start     → spawns a GENERATION thread that admits processes
              │                       (one every batch-process-freq CPU ticks)
              │
              ▼
        IScheduler (RR or FCFS)
              │  owns:
              │   • ready queue                    (processes waiting for a core)
              │   • SCHEDULER thread               (dispatch ready → idle core)
              │   • N CPUWorker threads            (one per core, execute instructions)
              │   • WATCHER/CLOCK thread           (free-running CPU tick + wake sleepers)
              │   • finished list
              ▼
        CPUWorker executes a Process's commands → each command updates the
        SymbolTable and appends log lines → Console `screen -r`/`process-smi` shows them.
```

### Threads alive during a run
| Thread | Count | Job |
|---|---|---|
| Console REPL | 1 (main) | Read commands, render `screen`/`process-smi`/`report-util` |
| Generation | 1 (while `scheduler-start` active) | Admit a new process every `batch-process-freq` ticks |
| Scheduler | 1 | Dispatch ready processes to idle cores |
| CPUWorker | `num-cpu` | Execute a process's instructions for one quantum |
| Watcher / clock | 1 | Advance the CPU tick (free-running) and wake sleeping processes |

---

## 3. Components

### `main.cpp`
- **Technical:** entry point; constructs `Console` and calls `run()`.
- **Now:** 4 lines, nothing else.

### `Console` (`Console.h/.cpp`) — the CLI
- **Technical:** the REPL. Tokenizes input, gates everything behind `initialize`, dispatches the
  spec commands. Owns the scheduler, the generator, the process **registry** (`name → Process`,
  mutex-guarded), and the generation thread.
- **Now — implemented commands:**
  - `initialize` — loads `config.txt`, builds the scheduler (RR/FCFS) + generator, starts the
    scheduler. Required before any other command (except `exit`).
  - `scheduler-start` / `scheduler-stop` — start/stop the generation thread.
  - `screen -s <name>` (create), `screen -r <name>` (re-attach), `screen -ls` (list).
  - `report-util` — writes the same status block to `csopesy-log.txt`.
  - The list (`screen -ls` / `report-util`) has three **state-driven** sections: **Running**
    (on a core), **Sleeping** (`WAITING`, scanned from the registry by state), and **Finished**.
  - `exit` — leaves a screen, or quits from the main menu.
  - Inside an attached screen: `process-smi` (refresh details + logs), `exit` (back to menu).
- **`process-smi` view:** process name, ID, a **Logs:** block (PRINT output only, per spec), then
  `Current instruction line` / `Lines of code` (or `Finished!`), then an **Instructions:** listing —
  the process's full program rendered as source (FOR shown with its body inline) where the current
  line is localized by a **colored, bracketed line number** (e.g. `[27]`). `screen -r` on a
  missing/finished process prints `Process <name> not found.`

### `SystemConfig` (`SystemConfig.h/.cpp`) — config.txt
- **Technical:** parses the 7 space-separated parameters and validates each against its range;
  returns a human-readable error on bad input.
- **Now:** all params parsed & range-checked: `num-cpu` [1,128], `scheduler` (fcfs/rr),
  `quantum-cycles` ≥1, `batch-process-freq` ≥1, `min-ins`/`max-ins` ≥1 with `max ≥ min`,
  `delays-per-exec` ≥0. Unknown keys / bad values are rejected.

### `Config.h` — compile-time constants
- **Technical:** small `constexpr` knobs (the runtime knobs live in `SystemConfig`).
- **Now:**
  - `EXEC_DELAY_MS = 100` — every PRINT sleeps this long so a run is observable to the eye.

### `ProcessGenerator` (`ProcessGenerator.h/.cpp`)
- **Technical:** builds dummy processes. Names them `p01, p02, …`. Each gets a **random** number
  of instructions in `[min-ins, max-ins]`, each instruction a **random** type from the six, with
  random operands (literals 0–65535 or variables from `{x,y,z,a,b}`). FOR bodies are 1–3 random
  commands; nesting is capped at depth 3 (spec).
- **Now:** fully randomized & spec-compliant. PRINT uses the default `"Hello world from <name>!"`.
  - Note: a separate quiz-6 variant (fixed `ADD/PRINT` set, `x/y/z`, `"Value from:"`) lives only on
    the `claude/fervent-faraday-wkeyku` branch — intentionally **not** here, to keep this generator
    requirement-compliant.

### `Process` (`Process.h/.cpp`) — the PCB
- **Technical:** holds pid, name, state (READY/RUNNING/WAITING/FINISHED), the command list +
  `commandCounter` (current instruction line), the `SymbolTable`, `coreId`, sleep request flag,
  and start/finish timestamps.
- **Now — plus the logging layer (new):** a mutex-guarded in-memory `logs` vector with
  `log()` (raw append), `getLogs()` (safe copy for the console), and `logMessage()` which wraps a
  message as `(timestamp) Core:<id> "<msg>"`. In-memory (no per-process files).

### `SymbolTable` (`SymbolTable.h`)
- **Technical:** `name → int` map for a process's variables; `getVariable` returns 0 for unknown
  names; `hasVariable` for the auto-declare check.
- **Now:** used by ADD/SUBTRACT/DECLARE/PRINT. Values are clamped to uint16 at the command level.

### `Operand` (`Operand.h/.cpp`)
- **Technical:** an ADD/SUBTRACT argument — either a literal uint16 or a variable name.
  `resolve()` reads the value (auto-declaring missing vars as 0, clamping to [0,65535]);
  `toString()` renders it for logs.
- **Now:** complete.

### Instructions — `ICommand` + the six commands
- **Technical:** `ICommand` is the base; each implements `execute(Process&)` (run it) and
  `toString()` (source text for the process-smi listing). Types: PRINT, DECLARE, ADD, SUBTRACT,
  SLEEP, FOR.
- **Now — each implemented and verified:**
  - **PRINT** — appends its message to the log (spec output). Supports the plain form
    (`"Hello world from p01!"`) and the interpolated form `PRINT("Value from: " + x)` resolved at
    execution time. Sleeps `EXEC_DELAY_MS`.
  - **DECLARE** — sets a variable. `toString()` → `DECLARE(var, value)`.
  - **ADD / SUBTRACT** — `dest = op2 ± op3`, saturating at 65535 / 0. `toString()` → `ADD(x, x, 4)`.
  - **SLEEP(X)** — sets a sleep request (X CPU ticks); the worker yields the core. → `SLEEP(X)`.
  - **FOR(body, repeats)** — runs its body inline `repeats` times (counts as one instruction line).
    `toString()` renders the body inline: `FOR([...], n)`. Nesting ≤ 3 enforced at generation.
  - Only PRINT writes to the Logs block; the other commands are shown in the **Instructions:**
    listing instead (via `toString()`), keeping Logs PRINT-only per spec.

### `IScheduler` (`IScheduler.h`)
- **Technical:** the interface workers and the console talk to (`addProcess`, `requeue`, `start`,
  `stop`, `moveToFinished`, `notifyScheduler`, waiting-list + tick methods, running/finished/core
  queries). Lets `CPUWorker` call back without knowing the policy.

### `SchedulerBase` (`SchedulerBase.h/.cpp`)
- **Technical:** shared base for both policies. Owns the **CPU tick counter**, the **waiting
  list**, and the **watcher thread**.
- **Now — the watcher is also the clock (new):** every 1 ms it `incrementTick()` (a **free-running
  clock**, per the spec's `while(running) cpuCycles++`) and re-admits any sleeping process whose
  wake tick has passed. Because the tick advances on its own — not per executed instruction — the
  clock never freezes when the ready queue drains or every process sleeps. This is what makes
  tick-driven generation and SLEEP timers deadlock-free.

### `RRScheduler` (`RRScheduler.h/.cpp`)
- **Technical:** Round-Robin. A `readyQueue`, a **scheduler thread** that waits for (work + an idle
  core) and assigns the front process to an idle `CPUWorker` built with `quantum = quantum-cycles`.
  Preempted processes are pushed to the **tail** via `requeue()`. Keeps a finished list.
- **Now:** complete; starts N workers + the scheduler loop + the watcher/clock; clean stop/join.

### `FCFSScheduler` (`FCFSScheduler.h/.cpp`)
- **Technical:** identical structure but workers run with `quantum = 0` → each process runs **to
  completion** before the core takes the next (no preemption).
- **Now:** complete; same lifecycle and reporting.

### `CPUWorker` (`CPUWorker.h/.cpp`) — one core
- **Technical:** a thread that waits to be `assign()`ed a process, then executes up to `quantum`
  instructions (0 = to completion), applying `delays-per-exec` ms before each. After the quantum it
  either: marks the process **finished**, **requeues** it (RR preemption), or parks it on the
  **waiting list** if it hit a SLEEP. Then goes idle and notifies the scheduler.
- **Now:** complete. Note: it no longer advances the CPU tick per instruction — the free-running
  clock owns the tick.

---

## 4. Configuration (`config.txt`)
```
num-cpu 4            # cores [1,128]
scheduler rr         # rr | fcfs
quantum-cycles 5     # RR time slice (instructions per turn)
batch-process-freq 1 # admit one process every N CPU ticks
min-ins 1000         # min instructions per process
max-ins 2000         # max instructions per process
delays-per-exec 0    # extra ms busy-wait before each instruction
```

---

## 5. Current status — requirements checklist (all verified)

| Requirement | Status |
|---|---|
| Commands PRINT/DECLARE/ADD/SUBTRACT/SLEEP/FOR | ✅ all functional |
| PRINT default `"Hello world from <name>!"` + variable form | ✅ |
| uint16 clamp [0,65535] & auto-declare missing vars to 0 | ✅ |
| FOR nesting ≤ 3 | ✅ |
| Processes run to completion (`Finished n/n`) | ✅ |
| `process-smi`: name, ID, logs, instruction line, `Finished!` | ✅ |
| `screen -r` missing/finished → `Process … not found.` | ✅ |
| `screen -ls` / `report-util` → `csopesy-log.txt` | ✅ |
| All 7 config params parsed + validated | ✅ |
| `batch-process-freq` measured in CPU ticks | ✅ (tick-faithful) |
| FCFS + RR both schedule and run the clock | ✅ |

---

## 6. Recent changes (this work)

1. **PRINT output + process-smi logs** (`9c32e50`) — `PrintCommand` was a stub; added the Process
   log buffer, real PRINT logging, and the `Logs:` section in `process-smi`.
2. **Tick-faithful `batch-process-freq`** (`fa07371`) — generation now admits per CPU-tick (was a
   wall-clock sleep coupled to `delays-per-exec`); the watcher became a free-running clock so this
   can't deadlock when the queue drains / a lone process sleeps.
3. **Instructions listing in process-smi** — each command implements `toString()`; `process-smi`
   now shows the full program with a `->` pointer at the current line, so the commands and current
   position are visible while **Logs** stays PRINT-only (spec). (This replaced an earlier
   per-command logging trace that wrote non-PRINT commands into the Logs block.)

---

## 7. Things to be aware of / decisions

- **Instructions listing length:** `process-smi` prints the *whole* program. For small processes
  this is ideal; with the default `min-ins/max-ins` (1000–2000) it prints 1000–2000 lines per
  refresh. Use modest instruction counts for demos, or switch to a windowed listing (current line
  ± N) if that becomes unwieldy.
- **CPU tick model:** the tick is a free-running ~1 ms clock, so `SLEEP(X)` ≈ X ms and
  `batch-process-freq X` ≈ one process per X ms. This matches the spec's free-running counter
  pseudocode and is internally consistent; it is *not* "one tick per executed instruction."
- **Sleeping visibility:** the `Sleeping` section is correct and state-driven, but at the default
  clock a SLEEP lasts only its tick count in ms (generator emits 1–5 ticks ⇒ ~1–5 ms), so a
  sleeper is rarely caught in a `screen -ls` snapshot — the section usually reads `(none)`. To make
  sleeps observable, slow the clock (the watcher's `sleep_for`) or generate longer SLEEPs.
- **Quiz-6 instruction set** (fixed `ADD/PRINT`, `x/y/z`, `"Value from:"`) is deliberately kept off
  this branch — it lives on `claude/fervent-faraday-wkeyku` as a temporary pre-recording edit so
  this deliverable stays requirement-compliant (randomized generation, default PRINT message).
- **`EXEC_DELAY_MS = 100`** on PRINT is what makes runs watchable; with `delays-per-exec 0` it is
  the main thing slowing execution.
