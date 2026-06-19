# CSOPESY FCFS — Developer Guide

How the Phase 1 implementation maps to the homework requirements and
OS scheduling theory. Read this before touching any source file.

---

## Requirement → implementation map

| Requirement | Where it lives |
|---|---|
| FCFS scheduler, 1 thread | `FCFSScheduler::schedulerLoop()` runs in `schedulerThread` |
| 1 thread per CPU core | `CPUWorker::workerLoop()` × 4, one per `CPUWorker` instance |
| 4 cores declared | `Config::NUM_CORES = 4` → `FCFSScheduler(Config::NUM_CORES)` in `main.cpp` |
| 10 processes at startup | `main.cpp` loop, `Config::NUM_PROCESSES = 10` |
| 100 print commands per process | `main.cpp` inner loop, `Config::PRINTS_PER_PROCESS = 100` |
| Per-process text file | `Process` opens `output/<name>.txt` in constructor when `ENABLE_FILE_LOGGING` |
| Timestamp + core in log line | `PrintCommand::execute()` calls `std::strftime` + `owner.getCoreId()` |
| `screen -ls` command | `Console::run()` dispatches to `Console::printProcessList()` |
| Running / finished lists | `FCFSScheduler::getRunningProcesses()` / `getFinishedProcesses()` |
| `exit` command | `Console::run()` breaks loop → `scheduler.stop()` → clean join |

---

## Architecture

```
main()
  │
  ├─ FCFSScheduler (1 thread: schedulerLoop)
  │     │
  │     ├─ readyQueue  ──────────────────── FIFO, arrival order
  │     │   (queueMutex + schedulerCv)
  │     │
  │     ├─ CPUWorker[0] (1 thread: workerLoop)   Core 0
  │     ├─ CPUWorker[1] (1 thread: workerLoop)   Core 1
  │     ├─ CPUWorker[2] (1 thread: workerLoop)   Core 2
  │     └─ CPUWorker[3] (1 thread: workerLoop)   Core 3
  │
  └─ Console (main thread: blocking readline loop)
```

Total threads at runtime: **6** (main + scheduler + 4 workers).

---

## Data flow — one process, start to finish

```
main()
  addCommand() × 100          ← loads PrintCommand list into PCB
  addProcess(p)               ← pushes to readyQueue, wakes schedulerCv

schedulerLoop() wakes
  finds idle CPUWorker
  readyQueue.pop()
  lock.unlock()
  worker.assign(p)            ← sets currentProcess, idle=false, wakes worker cv

workerLoop() wakes
  proc->setCoreId(id)
  proc->setState(RUNNING)     ← records startTime
  loop: executeCurrentCommand() → PrintCommand::execute()
          ├─ strftime timestamp
          ├─ owner.log(line)  ← writes to output/<name>.txt
          └─ sleep(EXEC_DELAY_MS)
        moveToNextLine()      ← commandCounter++
  proc->setState(FINISHED)    ← records finishTime
  scheduler.moveToFinished(p) ← appends to finishedList
  currentProcess = nullptr
  idle = true
  scheduler.notifyScheduler() ← wakes schedulerCv for next dispatch
```

---

## Thread synchronization

### Ready queue (`FCFSScheduler`)

```cpp
std::queue<std::shared_ptr<Process>> readyQueue;
mutable std::mutex                   queueMutex;
std::condition_variable              schedulerCv;
```

- `addProcess()` holds `queueMutex`, pushes, then notifies `schedulerCv`.
- `schedulerLoop()` waits on `schedulerCv` until there is both a queued
  process **and** at least one idle worker (or shutdown with empty queue).
- The lock is **released before** calling `worker.assign()` to avoid
  holding two locks at once.

### Worker slot (`CPUWorker`)

```cpp
std::shared_ptr<Process> currentProcess;   // null = idle
mutable std::mutex       mtx;
std::condition_variable  cv;
std::atomic<bool>        idle{true};
```

- `assign()` holds `mtx`, sets `currentProcess`, sets `idle = false`, notifies `cv`.
- `workerLoop()` waits on `cv` until `currentProcess != nullptr` or `running == false`.
- `getCurrentProcess()` (called by `Console`) acquires `mtx` for a safe snapshot.

### Finished list

```cpp
std::vector<std::shared_ptr<Process>> finishedList;
mutable std::mutex                    finishedMutex;
```

- `moveToFinished()` and `getFinishedProcesses()` both hold `finishedMutex`.

### Why no per-file mutex in `Process::log()`

Non-preemptive FCFS means a worker holds a process exclusively from
the moment it is assigned until `setState(FINISHED)`. No two threads
ever call `log()` on the same `Process` concurrently, so no mutex is needed.

---

## Class responsibilities

### `Config.h`
All tuning knobs in one place. Change values here; do not scatter magic
numbers in source files.

| Constant | Effect |
|---|---|
| `NUM_CORES` | Number of `CPUWorker` threads created |
| `NUM_PROCESSES` | How many processes `main()` spawns |
| `PRINTS_PER_PROCESS` | Instructions loaded per process |
| `EXEC_DELAY_MS` | Sleep per instruction — controls run duration |
| `ENABLE_FILE_LOGGING` | `true` for this homework; `false` for machine project |
| `OUTPUT_DIR` | Directory for `<name>.txt` log files (`"output"`) |
| `NAME_PREFIX` | Process name prefix (`"process_"` → `process_01` … `process_10`) |

### `ICommand` / `PrintCommand`
`ICommand` is the abstract instruction base. `PrintCommand` is the only
concrete command in Phase 1. `execute(Process& owner)` receives the PCB
so the command can read the current core id and write to the process log
without a global registry — this is a documented deviation from the
lecture stub.

### `Process` (PCB)
Holds everything the OS needs to manage one process: pid, name, state,
command list with a counter (program counter analogue), symbol table,
log file handle, and timestamps. `setState(RUNNING)` records `startTime`;
`setState(FINISHED)` records `finishTime`. These feed the `screen -ls`
display.

### `FCFSScheduler`
Owns the ready queue and the worker vector. `schedulerLoop()` is the
short-term scheduler: it wakes whenever the queue is non-empty **and**
a core is free, pops the front (FCFS = FIFO), and calls `worker.assign()`.
`addProcess()` is the long-term admission gate: processes arrive in
`main()` order, which becomes their FIFO position.

### `CPUWorker`
One instance per core. `workerLoop()` sleeps until assigned a process,
executes all of its commands without yielding (non-preemptive), then
reports back to the scheduler.

### `Console`
Runs on the main thread. Provides the boot animation (CSOPESY ASCII
logo + `[  OK  ]` messages) and the readline loop. `screen -ls` takes
a read-only snapshot from the scheduler and renders it; it does not
pause or affect scheduling.

---

## Why FCFS is non-preemptive here

`workerLoop()` executes every command before returning:

```cpp
while (!proc->isFinished()) {
    proc->executeCurrentCommand();
    proc->moveToNextLine();
}
```

There is no yield point, no check for a higher-priority process, and no
timer interrupt. A process runs to completion on the core it was
assigned to. This is the defining property of non-preemptive scheduling.

---

## Observed FCFS wave pattern (4 cores, 10 processes)

```
t=0s   Processes 1-4 dispatched to cores 0-3 simultaneously
t=10s  Processes 1-4 finish; processes 5-8 dispatched
t=20s  Processes 5-8 finish; processes 9-10 dispatched (2 cores idle)
t=30s  Processes 9-10 finish; all cores idle
```

`screen -ls` typed at t≈5s will show four running processes at ~50/100.
Typed at t≈15s it will show processes 5-8 running and 1-4 finished.
This visible progression is what the grader is checking for.

---

## How to change the number of processes or instructions

Edit `csopesy-fcfs/include/Config.h` — no other file needs touching:

```cpp
constexpr int NUM_CORES          = 4;    // CPU cores
constexpr int NUM_PROCESSES      = 10;   // processes created at boot
constexpr int PRINTS_PER_PROCESS = 100;  // instructions per process
constexpr int EXEC_DELAY_MS      = 100;  // ms per instruction
```

Rebuild after any change: `cmake --build build --parallel`

---

## Disabling file logging (machine project)

```cpp
// include/Config.h
constexpr bool ENABLE_FILE_LOGGING = false;
```

`Process::log()` becomes a no-op; the `output/` directory is never
created. All other behaviour is unchanged.

---

## Build quick reference

```bash
# Linux / Mac
cmake -S . -B build && cmake --build build --parallel
./build/csopesy

# Windows — MinGW (preferred, avoids MSVC threading issues)
cmake -S . -B build -G "MinGW Makefiles" && cmake --build build
.\build\csopesy.exe

# Windows — MSVC (must delete build/ before first configure)
cmake -S . -B build && cmake --build build
.\build\Debug\csopesy.exe
```

Commands at the `csosh` prompt:
- `screen -ls` — show running and finished processes
- `exit` — clean shutdown (waits for in-flight processes to finish)
