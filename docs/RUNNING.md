# Running CSOPESY OS Emulator

## Prerequisites

- [MSYS2](https://www.msys2.org/) installed
- MinGW-w64 compiler (via MSYS2)

### 1. Install CMake and GCC via MSYS2

Open the MSYS2 terminal (UCRT64) and run:

```bash
pacman -S mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-gcc
```

> If you use the MINGW64 environment instead, replace `ucrt64` with `x86_64`.

### 2. Add MSYS2 to your Windows PATH

Add `C:\msys64\ucrt64\bin` to your system PATH so PowerShell can find `cmake` and `g++`.

Verify in PowerShell:

```powershell
cmake --version
g++ --version
```

---

## First-time Build

Run these from the project root in PowerShell:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build --parallel
```

> The first configure step downloads GLFW and Dear ImGui via FetchContent — requires an internet connection.

---

## Run

```powershell
.\build\csopesy.exe
```

> **Note:** Close the app using the PWR button. Do not force-quit.

---

## Subsequent Builds

After making code changes, only the build step is needed:

```powershell
cmake --build build --parallel
.\build\csopesy.exe
```

Only re-run the configure step if you modify `CMakeLists.txt`.
