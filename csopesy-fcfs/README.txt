CSOPESY OS Emulator — Phase 1: FCFS Print-to-File
===================================================

ENTRY POINT
  src/main.cpp  (contains main())

BUILD
  cmake -S . -B build
  cmake --build build --parallel
  (No internet needed; all dependencies are standard C++17.)

RUN
  ./build/csopesy            # Linux / macOS
  build\Debug\csopesy.exe   # Windows (MSVC)

REQUIREMENTS
  Compiler : GCC 8+, Clang 7+, or MSVC 2019+ with C++17 enabled
  CMake    : 3.16 or newer
  Linux    : pthreads (part of glibc — no extra packages)
  Windows  : MSVC or MinGW-w64; pthreads handled automatically by CMake

OUTPUT
  Each process writes timestamped PRINT lines to  output/<name>.txt
  (the output/ directory is created automatically at runtime by Process.cpp
  once you implement the constructor).

CLI COMMANDS (Phase 1 only)
  screen -ls   Display CPU utilisation, running processes, finished processes.
  exit         Cleanly shut down (joins all threads, flushes log files).

FILE MAP
  include/Config.h          Named constants (no config.txt this phase)
  include/SymbolTable.h     Header-only PCB variable store
  include/ICommand.h        Abstract base command (deviation noted inside)
  include/PrintCommand.h    Concrete PRINT command
  include/Process.h         PCB declaration
  include/CPUWorker.h       One CPU core (worker thread)
  include/FCFSScheduler.h   Ready-queue + dispatcher
  include/Console.h         Minimal CLI
  src/main.cpp              Process factory + thread startup + shutdown
  src/PrintCommand.cpp      ICommand trivial defs + PrintCommand::execute()
  src/Process.cpp           *** TODO stubs — student implements every method ***
  src/CPUWorker.cpp         assign() implemented; workerLoop() is a TODO stub
  src/FCFSScheduler.cpp     addProcess/moveToFinished implemented; schedulerLoop() is a TODO stub
  src/Console.cpp           Fully implemented (screen -ls, exit)

WHAT THE STUDENT MUST IMPLEMENT (in this order)
  1. src/Process.cpp         — follow the STEP comments in each method
  2. CPUWorker::workerLoop() — follow the 10 STEP comments in CPUWorker.cpp
  3. FCFSScheduler::schedulerLoop() — follow the 7 STEP comments in FCFSScheduler.cpp

The project COMPILES and RUNS with only the stubs in place; processes simply
never execute and "screen -ls" shows empty lists until all three are done.

DESIGN NOTES (lecture mapping)
  Ready queue     FCFSScheduler::readyQueue  — FIFO; arrival order = FCFS order
  Short-term sched FCFSScheduler::schedulerLoop() — wakes when idle core + work
  CPU core        CPUWorker — one thread per core; non-preemptive execution loop
  PCB             Process — holds state, command list, symbol table, log file
  ICommand        Instruction abstraction; PRINT is the only type this phase

DEVIATION FROM LECTURE SPEC
  ICommand::execute() takes a Process& (not void) so PrintCommand can read the
  core ID and call log() without a global process registry.  Document this in
  your lab report and in the source comment inside ICommand.h.

GCC < 9 NOTE
  Once you add  #include <filesystem>  to Process.cpp, uncomment the stdc++fs
  block in CMakeLists.txt if your GCC version is below 9.
