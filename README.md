# CSOPESY Desktop OS Emulator

A desktop-style OS mockup built with **GLFW + OpenGL + Dear ImGui** in C++17.

## Prerequisites

| Tool | Minimum version |
|------|----------------|
| CMake | 3.16 |
| C++ compiler | C++17 (GCC 9 / Clang 10 / MSVC 2019) |
| OpenGL | 3.3 core profile (any modern GPU driver) |
| Git | any (FetchContent needs network on first configure) |
| Ninja (optional) | any — falls back to default generator if absent |

> **Note:** FetchContent downloads GLFW and Dear ImGui automatically on first `cmake` configure. Internet access required once.

## Build

### Command line
```bash
cmake -S . -B build
cmake --build build
./build/csopesy        # Linux/macOS
build\csopesy.exe      # Windows
```

### Visual Studio (Windows)
1. File → Open → Folder → select the repo root.
2. VS detects `CMakePresets.json` and shows the **default** preset.
3. Select **csopesy** as the startup item → Run/Debug.

### VS Code
Install the **CMake Tools** extension, then:
1. Open folder → select preset **Default (Debug)** → Build → Run.

## Controls

| Action | How |
|--------|-----|
| Open Task Manager | Taskbar button (middle) |
| Open File Explorer | Taskbar button (left) |
| Open System Info | Taskbar button (right of left cluster) |
| Shutdown / exit | **PWR** button on desktop or taskbar |

> The application **must** be closed via the PWR button (rubric requirement). The window is borderless fullscreen (no ✕ button), and Alt+F4 is suppressed via a GLFW close callback. The only valid exit path is the PWR button.

## Rubric components

- [x] **Component 1 — Desktop:** fullscreen wallpaper, live clock, PWR shutdown button
- [x] **Component 2 — Taskbar:** ≥3 icon buttons; 2 unique UI screens + Task Manager
- [x] **Component 3 — Task Manager:** process table with dummy CPU/memory values

## Project structure

```
src/
  core/          Application (GLFW+ImGui lifecycle), Clock
  compositor/    Compositor (per-frame layer renderer), Window base, WindowManager
  shell/         Desktop, Taskbar, BootSequence
  apps/          TaskManager, FileExplorerApp, SystemInfoApp
docs/            ARCHITECTURE.md, DEVELOPMENT_PLAN.md
assets/          wallpapers/, fonts/
```

## Team

CSOPESY Group — S.Y. 2025–2026
