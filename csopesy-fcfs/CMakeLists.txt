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
# MinGW on Windows ships in two thread-model variants:
#   • win32  — std::mutex / std::condition_variable ARE NOT implemented → link error
#   • posix  — full C++11 threading works correctly
# The two lines below force CMake to use the POSIX/pthread model on MinGW.
# If you still get "'mutex' is not a member of 'std'", reinstall MinGW-w64 and
# choose the "-posix" thread model (e.g. from https://winlibs.com).
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
target_link_libraries(csopesy PRIVATE Threads::Threads)

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
