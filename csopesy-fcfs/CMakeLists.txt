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

# std::thread requires pthreads on Linux/macOS.
find_package(Threads REQUIRED)
target_link_libraries(csopesy PRIVATE Threads::Threads)

# GCC < 9 ships std::filesystem in a separate library.
# Once you implement Process.cpp with #include <filesystem>, uncomment if needed:
# if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
#    CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
#     target_link_libraries(csopesy PRIVATE stdc++fs)
# endif()
