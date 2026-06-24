CSOPESY MO1 — Process Scheduler and CLI
=======================================

Group members:
  - <NAME 1>
  - <NAME 2>
  - <NAME 3>
  - <NAME 4>

Entry point:
  src/main.cpp  (contains main())

What this is:
  A standalone C++20 command-line OS emulator: a process multiplexer plus a
  command-line interpreter. Seeded from the Phase-1 FCFS demo (csopesy-fcfs/) and
  being extended to the full MO1 spec (FCFS + round-robin, config.txt-driven,
  full instruction set, screen multiplexer).

Build:
  cmake -S . -B build
  cmake --build build --parallel

Run:
  ./build/csopesy            (Linux / macOS)
  build\Debug\csopesy.exe    (Windows / MSVC)

Requirements:
  CMake >= 3.16, a C++20 compiler (GCC 11+ / Clang 13+ / MSVC 2022).

Configuration (config.txt, space-separated "key value" lines):
  num-cpu            number of CPU cores                  [1, 128]
  scheduler          "fcfs" or "rr"
  quantum-cycles     RR time slice (ignored for fcfs)     [1, 2^32-1]
  batch-process-freq new process every N CPU cycles       [1, 2^32-1]
  min-ins            min instructions per process         [1, 2^32-1]
  max-ins            max instructions per process         [1, 2^32-1], >= min-ins
  delays-per-exec    delay between instructions in cycles  [0, 2^32-1]

Commands (main menu):
  initialize         load + validate config.txt (run first)
  screen -s <name>   create a process and attach to its screen
  screen -r <name>   re-attach to an existing process's screen
  screen -ls         list CPU utilization + running/finished processes
  scheduler-start    continuously generate dummy processes
  scheduler-stop     stop generating dummy processes
  report-util        write a CPU utilization report to csopesy-log.txt
  exit               terminate the console

IMPLEMENTATION STATUS (scaffold):
  Implemented (seeded): FCFS engine, worker threads, PCB, PRINT, screen -ls, boot UI.
  Stubbed / TODO (search the source for "TODO(student)"):
    - SystemConfig::load / validate          (src/SystemConfig.cpp)
    - DECLARE / ADD / SUBTRACT / SLEEP / FOR  (src/*Command.cpp, src/Operand.cpp)
    - ProcessGenerator                        (src/ProcessGenerator.cpp)
    - RRScheduler (round-robin)               (src/RRScheduler.cpp)
    - Console handlers: initialize, screen -s/-r, scheduler-start/stop, report-util
                                              (src/Console.cpp)
