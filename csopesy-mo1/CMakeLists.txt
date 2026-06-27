cmake_minimum_required(VERSION 3.16)
project(csopesy-mo1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCES
    src/main.cpp
    # ── Process instructions (commands) ──────────────────────────────────────
    src/commands/PrintCommand.cpp
    src/commands/DeclareCommand.cpp
    src/commands/AddCommand.cpp
    src/commands/SubtractCommand.cpp
    src/commands/SleepCommand.cpp
    src/commands/ForCommand.cpp
    src/commands/Operand.cpp
    # ── Console / screen multiplexer (UI) ────────────────────────────────────
    src/console/Console.cpp
    src/console/ScreenManager.cpp
    src/console/MainMenuScreen.cpp
    src/console/ProcessScreen.cpp
    # ── Process / CPU ────────────────────────────────────────────────────────
    src/process/Process.cpp
    src/process/ProcessGenerator.cpp
    src/process/CPUWorker.cpp
    # ── Schedulers ───────────────────────────────────────────────────────────
    src/scheduler/SchedulerBase.cpp
    src/scheduler/FCFSScheduler.cpp
    src/scheduler/RRScheduler.cpp
    # ── Config ───────────────────────────────────────────────────────────────
    src/config/SystemConfig.cpp
)

add_executable(csopesy ${SOURCES})

# Component include dirs. Use -iquote (not -I) on non-MSVC so these dirs are only searched for
# quote-style includes (#include "Foo.h") — this prevents MinGW's pthread.h from resolving its
# #include <process.h> to our process/Process.h on a case-insensitive filesystem.
set(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/include/console
    ${CMAKE_CURRENT_SOURCE_DIR}/include/process
    ${CMAKE_CURRENT_SOURCE_DIR}/include/scheduler
    ${CMAKE_CURRENT_SOURCE_DIR}/include/config
)
if(MSVC)
    target_include_directories(csopesy PRIVATE ${INCLUDE_DIRS})
else()
    foreach(d ${INCLUDE_DIRS})
        target_compile_options(csopesy PRIVATE "-iquote${d}")
    endforeach()
endif()

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