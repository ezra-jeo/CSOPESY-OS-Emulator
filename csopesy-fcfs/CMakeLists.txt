cmake_minimum_required(VERSION 3.16)
project(csopesy-fcfs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    src/PrintCommand.cpp
    src/Process.cpp
    src/CPUWorker.cpp
    src/FCFSScheduler.cpp
    src/Console.cpp
)

add_executable(csopesy ${SOURCES})

# Let source files use bare #include "FileName.h" (no path prefix).
target_include_directories(csopesy PRIVATE include)

# ── Threading ────────────────────────────────────────────────────────────────
# MSVC has native Windows threading — std::thread works with no extra flags.
# Asking CMake to prefer pthreads on MSVC can confuse find_package(Threads)
# and break _beginthreadex inside <thread>, so we skip it there.
# MinGW needs the POSIX thread model; win32-model MinGW won't compile <mutex>.
if(NOT MSVC)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
endif()
find_package(Threads REQUIRED)
target_link_libraries(csopesy PRIVATE Threads::Threads)

if(MSVC)
    # Use the dynamic multithreaded CRT (/MD, /MDd) which provides _beginthreadex.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:Debug>:Debug>")
    # Encode source and execution as UTF-8 so the block-letter logo renders correctly.
    # /FS forces synchronous PDB writes so parallel cl.exe invocations don't race.
    target_compile_options(csopesy PRIVATE /utf-8 /FS)
endif()

if(MINGW)
    # -mthreads enables MinGW's Win32-safe thread-local storage for exceptions.
    target_compile_options(csopesy PRIVATE -mthreads)
    target_link_options(csopesy   PRIVATE -mthreads)
endif()

# GCC < 9 ships std::filesystem in a separate library.
# Once you implement Process.cpp with #include <filesystem>, uncomment if needed:
# if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
#    CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
#     target_link_libraries(csopesy PRIVATE stdc++fs)
# endif()
