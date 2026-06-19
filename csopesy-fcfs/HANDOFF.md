# CSOPESY FCFS — AI Handoff Document

This document gives a new model session enough context to continue work
on `csopesy-fcfs/` without re-reading the full conversation history.

---

## What this sub-project is

A standalone C++17 CLI program inside the `csopesy-fcfs/` subdirectory
of `CSOPESY-OS-Emulator`. It is **not** the same project as the root
CMakeLists.txt (which builds a separate Dear-ImGui GUI). The two share
the same git repository but are independent CMake projects.

**Purpose:** Phase 1 FCFS homework for a De La Salle University OS course.
Simulate 4 CPU cores running 10 processes (each with 100 print instructions)
under a non-preemptive First-Come-First-Serve scheduler, log every instruction
to a per-process text file, and expose a `screen -ls` command that shows
running/finished processes.

---

## Branch layout

| Branch | Contents |
|---|---|
| `feat/cli` | Student scaffold — `Process.cpp`, `CPUWorker.cpp`, `FCFSScheduler.cpp` have step-by-step TODO stubs. Students fill these in. |
| `feat/cli-complete` | Reference solution — all stubs implemented. Use to compare or demo. |
| `claude/tender-euler-zhz7bm` | Original scaffolding branch; merged into `feat/cli`. |

Development branch for this session: **`claude/tender-euler-zhz7bm`** (per session config).
All recent work was committed directly to `feat/cli` and `feat/cli-complete` instead,
because the session-config branch was used as the merge base.

---

## File map

```
csopesy-fcfs/
├── CMakeLists.txt          — build; has MSVC/MinGW/GCC guards
├── include/
│   ├── Config.h            — ALL tuning constants (cores, processes, delay, logging flag)
│   ├── ICommand.h          — abstract base; execute(Process&) passes the PCB
│   ├── PrintCommand.h      — concrete command: write timestamped line to log
│   ├── Process.h           — PCB: state, command list, log file, timestamps
│   ├── FCFSScheduler.h     — ready queue (FIFO) + scheduler thread
│   ├── CPUWorker.h         — one worker thread per core
│   ├── Console.h           — CLI loop
│   └── SymbolTable.h       — named int variables (not used Phase 1)
└── src/
    ├── main.cpp            — creates 10 processes, loads commands, starts scheduler, runs console
    ├── PrintCommand.cpp    — fully implemented; sleeps EXEC_DELAY_MS after writing
    ├── Process.cpp         — student TODO on feat/cli; complete on feat/cli-complete
    ├── CPUWorker.cpp       — student TODO (workerLoop) on feat/cli; complete on feat/cli-complete
    ├── FCFSScheduler.cpp   — student TODO (schedulerLoop) on feat/cli; complete on feat/cli-complete
    └── Console.cpp         — fully implemented; CSOPESY ASCII boot UI + screen -ls
```

---

## Key design decisions (already made — do not revisit)

1. **Circular include** between `CPUWorker` and `FCFSScheduler` is broken by
   forward-declaring each class in the other's header; full `#include` only in `.cpp`.

2. **`execute(Process& owner)`** — `ICommand::execute` takes the PCB so
   `PrintCommand` can call `owner.getCoreId()` and `owner.log()` without a
   global registry. Documented as a deviation from the lecture stub.

3. **No per-file mutex in `Process::log()`** — non-preemptive FCFS guarantees
   exactly one worker owns a process from start to finish; concurrent writes
   to the same `ofstream` are impossible.

4. **`EXEC_DELAY_MS = 100`** — raised from 20 ms so the 30-second run gives the
   grader time to type `screen -ls` repeatedly and observe three FCFS waves
   (processes 1–4, then 5–8, then 9–10).

5. **`ENABLE_FILE_LOGGING = true`** for this homework (the submitted ZIP must
   contain 10 text files). The machine project must flip this to `false`.

---

## Build

```bash
# Linux / MinGW (preferred on Windows — avoids MSVC PDB/CRT issues)
cmake -S csopesy-fcfs -B csopesy-fcfs/build -G "MinGW Makefiles"  # Windows only
cmake -S csopesy-fcfs -B csopesy-fcfs/build                        # Linux/Mac
cmake --build csopesy-fcfs/build --parallel
./csopesy-fcfs/build/csopesy

# MSVC (if you must)
# Must delete build/ and regenerate — cached CMakeCache.txt will use old flags.
# /FS flag (already in CMakeLists.txt) prevents parallel PDB write race (C1041).
# cmake -S csopesy-fcfs -B csopesy-fcfs/build && cmake --build csopesy-fcfs/build
```

---

## Known MSVC issues (already fixed in CMakeLists.txt)

| Error | Cause | Fix already applied |
|---|---|---|
| `C3861: '_beginthreadex' not found` | `CMAKE_THREAD_PREFER_PTHREAD` interferes with MSVC CRT | Wrapped in `if(NOT MSVC)`; added `MultiThreadedDLL` runtime |
| `C1041: cannot open .pdb` | Parallel `cl.exe` workers race on one PDB file | `/FS` added to `target_compile_options` |

Both fixes require a **clean regeneration** (`delete build/`, then `cmake -S . -B build`).
Deleting only `CMakeCache.txt` is sometimes enough but not always.

---

## What the student must implement (feat/cli stubs)

Three functions, each with numbered STEP comments in the source:

| File | Function | Core concept |
|---|---|---|
| `src/Process.cpp` | Constructor (file logging), `addCommand`, `executeCurrentCommand`, `moveToNextLine`, `isFinished`, `setState`, `setCoreId`, `log` | PCB management |
| `src/CPUWorker.cpp` | `workerLoop()` | Worker thread: wait → execute all instructions → report done |
| `src/FCFSScheduler.cpp` | `schedulerLoop()` | Dispatcher: wait for idle core + queued process → assign → repeat |

The complete implementations are on `feat/cli-complete` for direct comparison.

---

## Output format (must match grading rubric)

File: `output/process_01.txt`
```
Process name: process_01
Logs:

(08/06/2024 09:15:22AM) Core:0 "Hello world from process_01!"
(08/06/2024 09:15:28AM) Core:0 "Hello world from process_01!"
...
```

`screen -ls` output shows:
- CPU utilization %, cores used/available
- Running processes: name | start timestamp | Core:N | counter/total
- Finished processes: name | finish timestamp | "Finished" | total/total

---

## What comes next (Phase 2 / machine project)

- Read scheduler config from `config.txt` instead of `Config.h` constants
- Support more command types beyond PRINT (IO, etc.)
- Disable file logging (`ENABLE_FILE_LOGGING = false`)
- Possibly extend to Round Robin or other policies
