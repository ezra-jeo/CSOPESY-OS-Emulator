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
  Linux / macOS:
    cmake -S . -B build
    cmake --build build --parallel

  Windows (MinGW-w64 via MSYS2, POSIX threads):
    cmake --preset mingw
    cmake --build --preset mingw
    (One-time: if a build/ dir already exists from a previous MSVC configure,
     delete it first with  rmdir /s /q build  -- CMake caches the generator
     per build dir and cannot switch MSVC -> MinGW in place.)

Run:
  ./build/csopesy            (Linux / macOS)
  build\csopesy.exe          (Windows / MinGW -- "MinGW Makefiles" is a
                              single-config generator, so the exe lands
                              directly in build\, not build\Debug\)

Requirements:
  CMake >= 3.21 (for CMakePresets), a C++20 compiler (GCC 11+ / Clang 13+).
  On Windows, MinGW-w64 with the POSIX thread model (e.g. MSYS2 ucrt64/mingw64);
  the win32 thread model cannot compile <thread>/<mutex>.

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
