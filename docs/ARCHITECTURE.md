# CSOPESY OS Emulator — Architecture

## Overview

The emulator is a **single-threaded, immediate-mode GUI application**. There is no real
scheduler, process model, or OS kernel — it is a *visual mockup* rendered at ~60 fps.

## Per-frame pipeline

```
┌─────────────────────────────────────────────────────────────┐
│                     main loop (vsync)                       │
│                                                             │
│  glfwPollEvents()                                           │
│         │                                                   │
│  dt = glfwGetTime() delta                                   │
│         │                                                   │
│  ImGui_ImplOpenGL3_NewFrame()                               │
│  ImGui_ImplGlfw_NewFrame()                                  │
│  ImGui::NewFrame()                                          │
│         │                                                   │
│  Compositor::render(dt)   ◄── the "compositor"              │
│    │                                                        │
│    ├─ [boot not done] BootSequence::update(dt)+draw()       │
│    │     Bios(1.8s) → Splash(1.8s) → Loading(2s) → Done    │
│    │     any key / mouse click skips immediately           │
│    │                                                        │
│    ├─ Desktop::draw()         fullscreen wallpaper layer    │
│    │    ├─ PNG texture (stb_image) or gradient fallback     │
│    │    ├─ corner clock (Clock::now())                      │
│    │    └─ PWR button → confirmation modal → requestQuit()  │
│    │                                                        │
│    ├─ WindowManager::drawWindows()                          │
│    │    ├─ TaskManager::draw()     (if open)                │
│    │    │    sortable table, End Task, Performance graphs   │
│    │    ├─ FileExplorerApp::draw() (if open)                │
│    │    │    per-dir file lists, search filter              │
│    │    └─ SystemInfoApp::draw()   (if open)                │
│    │         animated CPU/Mem bars, volume slider           │
│    │                                                        │
│    └─ Taskbar::draw()       fixed bottom panel              │
│         ├─ [F] → focus/toggle FileExplorerApp               │
│         ├─ [I] → focus/toggle SystemInfoApp                 │
│         ├─ [T] → focus/toggle TaskManager                   │
│         │    (button highlighted when window is focused)    │
│         ├─ live clock                                       │
│         └─ [PWR] → Application::requestQuit()              │
│                                                             │
│  ImGui::Render()                                            │
│  ImGui_ImplOpenGL3_RenderDrawData(...)                      │
│  glfwSwapBuffers()                                          │
└─────────────────────────────────────────────────────────────┘
```

## Class responsibilities

| Class | File | Responsibility |
|-------|------|----------------|
| `Application` | `core/Application` | GLFW window + GL context + ImGui init/shutdown + main loop |
| `Clock` | `core/Clock` | Returns formatted current local time string |
| `Texture` | `core/Texture` | stb_image wrapper: load PNG → OpenGL texture id |
| `Theme` | `core/Theme` | ImGuiStyle tweaks for retro blue-dark OS palette |
| `Compositor` | `compositor/Compositor` | Calls BootSequence, Desktop, WindowManager, Taskbar in order |
| `Window` | `compositor/Window` | Abstract base: title, open/focused flags, `requestFocus()`, virtual `draw()` |
| `WindowManager` | `compositor/WindowManager` | Owns list of `Window*`; calls `draw()` on open windows |
| `BootSequence` | `shell/BootSequence` | State machine: Bios → Splash → Loading → Done; skippable |
| `Desktop` | `shell/Desktop` | Fullscreen background + clock + PWR button + confirm modal |
| `Taskbar` | `shell/Taskbar` | Fixed panel + icon buttons (highlighted when window focused) |
| `TaskManager` | `apps/TaskManager` | Sortable process table, End Task, Performance graph tab |
| `FileExplorerApp` | `apps/FileExplorerApp` | Two-panel explorer with per-dir files + search filter |
| `SystemInfoApp` | `apps/SystemInfoApp` | Animated CPU/Mem/Disk meters, network info, volume slider |

## State model

All state is plain C++ owned by `Application` and passed by pointer/reference.
There is **no render thread, no message queue** beyond ImGui's internal draw list.
"Processes" in Task Manager are `std::vector<ProcessRow>` — no actual process enumeration.

## Key design decisions

- **Immediate-mode UI (Dear ImGui):** every widget is declared anew each frame; no
  persistent widget objects. This mirrors how real compositors redraw the screen every
  frame and keeps the code extremely simple.
- **FetchContent dependencies:** GLFW, ImGui, and stb are downloaded at CMake configure
  time, so the repo stays small and reproducible across machines.
- **Boot gate:** `Compositor::render(dt)` early-returns after rendering boot screens until
  `BootSequence::isDone()`, so the rest of the pipeline never sees a partial frame.
- **Single executable:** the entire mockup ships as one `csopesy` binary; assets are
  copied next to it by a CMake POST_BUILD rule.
