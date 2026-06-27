CSOPESY MO1 — Process Scheduler and CLI
=======================================

Group 9 members:
- Ronald Dawson Catignas
- Jose Paolo Cruzado
- Ezra Jeonadab Del Rosario
- Lorenzo Alfred Nery

Entry point:
  src/main.cpp  (contains main())

What this is:
  A standalone C++20 command-line OS emulator: a process multiplexer plus a
  command-line interpreter. Continued from the Phase-1 FCFS demo (csopesy-fcfs/) and
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

    The preset pins the compiler to C:/msys64/ucrt64/bin/g++.exe so PATH order
    can't pick a stray g++. If you installed MSYS2 somewhere else, edit that
    path in CMakePresets.json.

    Troubleshooting -- errors *inside* GCC's own headers, e.g.
      bits/fstream.tcc: 'std::ios_base::openmode' has not been declared
    mean g++ is pulling stale headers from a different toolchain. Check for a
    polluted include path:
      where.exe g++                 (more than one hit = a rogue g++)
      echo $env:CPATH $env:CPLUS_INCLUDE_PATH $env:C_INCLUDE_PATH
    Clear any *_INCLUDE_PATH / CPATH var that points outside C:\msys64, remove
    any old MinGW from PATH, then delete build/ and reconfigure. Building from
    the "MSYS2 UCRT64" shell (clean environment) sidesteps this entirely.

Run:
  ./build/csopesy            (Linux / macOS)
  .\build\csopesy.exe        (Windows / MinGW -- "MinGW Makefiles" is a
                              single-config generator, so the exe lands
                              directly in build\, not build\Debug\.
                              Keep the leading .\  -- PowerShell will not
                              run a bare relative path like build\csopesy.exe)

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