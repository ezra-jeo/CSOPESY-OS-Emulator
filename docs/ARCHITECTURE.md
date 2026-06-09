# CSOPESY OS Emulator — Architecture

## Overview

The emulator is a **single-threaded, immediate-mode GUI application**. There is no real
scheduler, process model, or OS kernel — it is a *visual mockup* rendered at ~60 fps.

## Per-frame pipeline

```
┌─────────────────────────────────────────────────────────┐
│                     main loop (vsync)                   │
│                                                         │
│  glfwPollEvents()                                       │
│         │                                               │
│  ImGui_ImplOpenGL3_NewFrame()                           │
│  ImGui_ImplGlfw_NewFrame()                              │
│  ImGui::NewFrame()                                      │
│         │                                               │
│  Compositor::render()   ◄── the "compositor"            │
│    ├─ Desktop::draw()        fullscreen wallpaper layer │
│    │    ├─ gradient / texture background                │
│    │    ├─ corner clock (Clock::now())                  │
│    │    └─ PWR button  → Application::requestQuit()    │
│    │                                                    │
│    ├─ WindowManager::drawWindows()                      │
│    │    ├─ TaskManager::draw()     (if open)            │
│    │    ├─ FileExplorerApp::draw() (if open)            │
│    │    └─ SystemInfoApp::draw()   (if open)            │
│    │                                                    │
│    └─ Taskbar::draw()      fixed top/bottom panel       │
│         ├─ [📁] → toggles FileExplorerApp               │
│         ├─ [💻] → toggles SystemInfoApp                 │
│         └─ [📊] → toggles TaskManager                  │
│         └─ [PWR] → Application::requestQuit()          │
│                                                         │
│  ImGui::Render()                                        │
│  ImGui_ImplOpenGL3_RenderDrawData(...)                  │
│  glfwSwapBuffers()                                      │
└─────────────────────────────────────────────────────────┘
```

## Class responsibilities

| Class | File | Responsibility |
|-------|------|----------------|
| `Application` | `core/Application` | GLFW window + GL context + ImGui init/shutdown + main loop |
| `Clock` | `core/Clock` | Returns formatted current local time string |
| `Compositor` | `compositor/Compositor` | Calls Desktop, WindowManager, Taskbar in correct draw order |
| `Window` | `compositor/Window` | Abstract base: title, open flag, virtual `draw()` |
| `WindowManager` | `compositor/WindowManager` | Owns list of `Window*`; calls `draw()` on open windows |
| `Desktop` | `shell/Desktop` | Fullscreen background + clock + PWR button |
| `Taskbar` | `shell/Taskbar` | Fixed panel + icon buttons that toggle windows |
| `BootSequence` | `shell/BootSequence` | (Phase 5) State machine: BIOS → splash → desktop |
| `TaskManager` | `apps/TaskManager` | Process table with dummy CPU/memory rows |
| `FileExplorerApp` | `apps/FileExplorerApp` | Unique UI screen #1 |
| `SystemInfoApp` | `apps/SystemInfoApp` | Unique UI screen #2 |

## State model

All state is plain C++ owned by `Application` and passed by pointer/reference.
There is **no render thread, no message queue, no dynamic memory allocation** beyond
ImGui's internal draw list. "Processes" in Task Manager are `std::vector<ProcessRow>`
with hard-coded dummy values — no actual process enumeration.

## Key design decisions

- **Immediate-mode UI (Dear ImGui):** every widget is declared anew each frame; there
  are no persistent widget objects. This mirrors how real compositors redraw the screen
  every frame and keeps the code extremely simple.
- **FetchContent dependencies:** GLFW and ImGui are downloaded at CMake configure time,
  so the repo stays small and reproducible across machines.
- **Single executable:** the entire mockup ships as one `csopesy` binary; no DLLs or
  runtime data files beyond optional font/texture assets.
