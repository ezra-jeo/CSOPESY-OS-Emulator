cmake_minimum_required(VERSION 3.16)
project(csopesy-mo1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    # ── Seeded FCFS base (working) ───────────────────────────────────────────
    src/Process.cpp
    src/PrintCommand.cpp
    src/CPUWorker.cpp
    src/FCFSScheduler.cpp
    src/Console.cpp
    # ── MO1 additions (scaffold / to be implemented) ─────────────────────────
    src/SystemConfig.cpp
    src/Operand.cpp
    src/DeclareCommand.cpp
    src/AddCommand.cpp
    src/SubtractCommand.cpp
    src/SleepCommand.cpp
    src/ForCommand.cpp
    src/ProcessGenerator.cpp
    src/RRScheduler.cpp
)

add_executable(csopesy ${SOURCES})

# Let source files use bare #include "FileName.h" (no path prefix).
target_include_directories(csopesy PRIVATE include)

# ── Threading ────────────────────────────────────────────────────────────────
# MSVC has native Windows threading — std::thread works with no extra flags.
# MinGW needs the POSIX thread model; win32-model MinGW won't compile <mutex>.
if(NOT MSVC)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
endif()
find_package(Threads REQUIRED)
target_link_libraries(csopesy PRIVATE Threads::Threads)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:Debug>:Debug>")
    target_compile_options(csopesy PRIVATE /utf-8 /FS)
endif()

if(MINGW)
    target_compile_options(csopesy PRIVATE -mthreads)
    target_link_options(csopesy   PRIVATE -mthreads)
endif()
