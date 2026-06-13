# CSOPESY OS Emulator — Architecture

> **See also:** `docs/COMPONENTS.md` for a plain-language + technical explanation of every class.

---

## 1. Technology Stack

```
┌──────────────────────────────────────────────────────────────────┐
│                      CSOPESY Application                         │
│                        (C++20, single thread)                    │
├──────────────────────────────────────────────────────────────────┤
│                       Dear ImGui v1.91.6                         │
│           Immediate-mode GUI — all widgets, windows, graphs      │
├───────────────────────┬──────────────────────────────────────────┤
│     GLFW 3.4          │          stb_image                       │
│  Window · Input ·     │    PNG/JPG decoder → raw RGBA bytes      │
│  OpenGL context       │                                          │
├───────────────────────┴──────────────────────────────────────────┤
│                       OpenGL 3.3 Core                            │
│              GPU draw calls · Texture upload · GLSL              │
└──────────────────────────────────────────────────────────────────┘
         All three dependencies fetched at CMake configure time
                  via FetchContent (no manual DLL setup)
```

---

## 2. Layer Ownership

Objects shown in bold own (via `unique_ptr` or member value) the objects below them.

```
Application
│  (owns GLFW window handle, ImGui context)
│
└── Compositor                          [member of Application's run() scope]
    │
    ├── BootSequence                    [value member of Compositor]
    │
    ├── Desktop                         [static methods — no state]
    │
    ├── WindowManager                   [value member of Compositor]
    │   ├── unique_ptr<TaskManager>
    │   ├── unique_ptr<FileExplorerApp>
    │   └── unique_ptr<SystemInfoApp>
    │
    └── Taskbar                         [static methods — no state]
         (holds raw non-owning pointers to the three apps above)
```

**Lifetime rule:** Everything lives for the duration of `Application::run()`.
There are no dynamic allocations beyond the three `unique_ptr` app windows.
The Compositor holds raw (non-owning) pointers to those windows solely to
pass them to `Taskbar::draw()`.

---

## 3. Class Inheritance

```
                    compositor::Window   (abstract base)
                    ┌──────┬──────────┐
                    │      │          │
             TaskManager  FileExplorerApp  SystemInfoApp
             (apps/)      (apps/)          (apps/)

Window interface:
  + draw()              [pure virtual — each app implements]
  + isOpen()
  + isFocused()
  + requestFocus()
  + toggle()
  # title_              [protected state shared by all windows]
  # open_
  # focused_
  # focusRequested_
```

Desktop and Taskbar are **not** Windows — they are pure static-method classes
with no base class because they are always present and never toggled.

---

## 4. Per-Frame Render Pipeline

Every monitor refresh (~16 ms at 60 Hz) the following executes in order:

```
┌─────────────────────────────────────────────────────────────────────┐
│  Application::run()  ─  main loop                                   │
│                                                                      │
│  ① glfwPollEvents()                                                  │
│     │  OS delivers: key presses, mouse movement, resize events       │
│     ▼                                                                │
│  ② dt = glfwGetTime() - lastTime                                     │
│     │  High-resolution elapsed seconds since last frame              │
│     ▼                                                                │
│  ③ ImGui::NewFrame()                                                 │
│     │  ImGui resets its draw command buffer                          │
│     │  ImGui reads GLFW input state (cursor pos, buttons, keys)      │
│     ▼                                                                │
│  ④ Compositor::render(dt)          ◄── all application logic here    │
│     │                                                                │
│     │   if boot not done ──────────────────────────────────────┐    │
│     │       BootSequence::update(dt)                           │    │
│     │       BootSequence::draw()                               │    │
│     │       return  ◄── pipeline stops here during boot        │    │
│     │                                                          │    │
│     │   [after boot]                                           │    │
│     │                                                          │    │
│     ├── Desktop::draw()           LAYER 1 — background        │    │
│     │                                                          │    │
│     ├── WindowManager::drawWindows()   LAYER 2 — apps         │    │
│     │     calls draw() on each window                         │    │
│     │                                                          │    │
│     └── Taskbar::draw()           LAYER 3 — always on top     │    │
│              calls BringWindowToDisplayFront()  ──────────────┘    │
│                                                                      │
│  ⑤ ImGui::Render()                                                   │
│     │  Compiles all draw commands into a GPU vertex/index buffer     │
│     ▼                                                                │
│  ⑥ glClear()  +  ImGui_ImplOpenGL3_RenderDrawData()                 │
│     │  Sends buffer to GPU — OpenGL draws every pixel                │
│     ▼                                                                │
│  ⑦ glfwSwapBuffers()                                                 │
│     │  Flip front ↔ back buffer → frame appears on screen           │
│     ▼                                                                │
│  back to ①                                                           │
└─────────────────────────────────────────────────────────────────────┘
```

> **Why this order?**  ImGui collects draw commands in sequence. Whatever is
> issued first appears visually behind everything issued after. Desktop must
> be first (fills the whole screen). Taskbar must be last (always on top).

---

## 5. Boot Sequence State Machine

```
  startup
     │
     ▼
  ┌────────┐   timer >= 1.8s    ┌────────┐   timer >= 1.8s    ┌─────────┐   timer >= 2.0s    ┌──────┐
  │  BIOS  │──────────────────► │ SPLASH │──────────────────► │ LOADING │──────────────────► │ DONE │
  └────────┘                    └────────┘                    └─────────┘                    └──────┘
      │                             │                              │
      │  any key / mouse click      │                              │
      └─────────────────────────────┴──────────────────────────────┘
                                    │
                                    ▼
                             skip to DONE

  BIOS screen:    CPU/memory text, "Press any key to skip"
  SPLASH screen:  Box-drawing CSOPESY logo (U+2500–U+259F glyphs) + subtitle
  LOADING screen: Progress bar driven by (timer / 2.0s)
  DONE:           isDone() = true — Compositor unblocks, never calls boot again
```

---

## 6. Window Focus Handshake

Sequence across two frames when a user clicks a Taskbar button for an open window:

```
  Frame N                              Frame N+1
  ─────────────────────────────────    ─────────────────────────────────────
  User clicks [F] Files button

  Taskbar::draw()
    fileExp.isOpen() == true
    → fileExp.requestFocus()
        open_          = true   ──────────────────────────────────────────►
        focusRequested_= true   ──────────────────────────────────────────►

                                       FileExplorerApp::draw()
                                         sees focusRequested_ == true
                                         → ImGui::SetNextWindowFocus()
                                           (must be BEFORE Begin())
                                         → focusRequested_ = false
                                         → ImGui::Begin(...)
                                             window opens with focus
                                         → focused_ = IsWindowFocused(...)
                                             returns true

                                       Taskbar::draw()
                                         fileExp.isFocused() == true
                                         → pushButtonColors(true)
                                           button renders bright blue  ◄──
```

> The two-frame delay is **required** by ImGui's API: `SetNextWindowFocus()`
> must be called before `Begin()`, so the flag bridges the gap between
> the Taskbar (which detects the click) and the app window (which calls Begin).

---

## 7. Compositor Draw Stack (Visual Z-Order)

What you see on screen, from back to front:

```
  ┌─────────────────────────────────────────────────┐  ▲
  │  Taskbar  (42px, bottom, always on top)         │  │  drawn last
  ├─────────────────────────────────────────────────┤  │  (highest z)
  │                                                 │  │
  │   ┌──────────────────┐  ┌────────────────────┐  │  │
  │   │   Task Manager   │  │   File Explorer    │  │  │
  │   │   (floating)     │  │   (floating)       │  │  │
  │   └──────────────────┘  └────────────────────┘  │  │
  │              ┌──────────────────┐                │  │
  │              │   System Info    │                │  │  app windows
  │              │   (floating)     │                │  │  (layer 2)
  │              └──────────────────┘                │  │
  │                                                 │  │
  │  Desktop background (wallpaper or gradient)     │  │  drawn first
  └─────────────────────────────────────────────────┘  ▼  (lowest z)
```

Floating windows can be dragged anywhere on the desktop area.
They cannot overlap the Taskbar because Taskbar calls
`BringWindowToDisplayFront()` every frame, which moves it to the
front of ImGui's internal window stack regardless of mouse interaction.

---

## 8. WindowManager Memory Model

```
  WindowManager
  ┌──────────────────────────────────────────────────┐
  │  windows_  : vector<unique_ptr<Window>>           │
  │                                                  │
  │  [0]  unique_ptr ──────────────► TaskManager     │
  │  [1]  unique_ptr ──────────────► FileExplorerApp │
  │  [2]  unique_ptr ──────────────► SystemInfoApp   │
  └──────────────────────────────────────────────────┘
         ▲                   ▲                 ▲
         │ raw ptr           │ raw ptr         │ raw ptr
         │ (non-owning)      │ (non-owning)    │ (non-owning)
  ┌──────┴───────────────────┴─────────────────┴──────┐
  │  Compositor                                       │
  │  taskManager_*   fileExplorer_*   sysInfo_*       │
  └───────────────────────────────────────────────────┘
         │                   │                 │
         └───────────────────┴─────────────────┘
                             │
                             ▼
                      Taskbar::draw(app, *taskMgr, *fileExp, *sysInfo)
                      (reads isFocused(), calls requestFocus() / toggle())
```

`add<T>()` uses a **variadic template** with perfect forwarding so any window
type can be registered without the WindowManager knowing the concrete type:
```cpp
template<typename T, typename... Args>
T* add(Args&&... args) {
    auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
    T* raw   = ptr.get();
    windows_.push_back(std::move(ptr));
    return raw;   // caller keeps non-owning ptr; vector owns the object
}
```

---

## 9. Texture / Wallpaper Pipeline

```
  Disk                     CPU                          GPU
  ─────                    ───                          ───
  wallpaper.png
       │
       ▼
  stbi_load()         →  unsigned char[]   →   glTexImage2D()  →  Texture.id
  (decode PNG/JPG)       raw RGBA bytes        (upload to VRAM)    (GPU handle)
                                │
                          stbi_image_free()
                          (free CPU copy —
                           GPU keeps its own)

  Every frame:
  ImDrawList::AddImage(Texture.id, {0,0}, {W,H})
       │
       └── GPU samples texture at full screen size → wallpaper visible
```

If `stbi_load` fails (file missing, wrong format):
- `stbi_failure_reason()` is printed to stderr
- `Texture.id` stays 0 → `valid()` returns false
- Desktop falls back to the animated navy gradient

---

## 10. Performance Graph Ring Buffer

Task Manager's CPU and Memory graphs store 9 seconds of history at 10 Hz:

```
  cpuHist_[90]  — circular buffer, write head = histOffset_

  oldest                                         newest
    │                                               │
    ▼                                               ▼
  [ 23.1 | 24.0 | 22.8 | ... | 25.3 | 24.1 | 23.7 ]
     ▲                                         ▲
     histOffset_                          histOffset_-1
     (next write)                         (last written)

  Every 0.1s (gated by plotAccum_ accumulator):
    cpuHist_[histOffset_] = sum of all process CPU values
    histOffset_ = (histOffset_ + 1) % 90   — wraps at end

  PlotLines("##cpu", cpuHist_, 90, histOffset_, label, 0, 100, size)
                                   ▲
                                   tells ImGui where the oldest sample is
                                   so it draws left-to-right correctly
```

---

## 11. Class Responsibilities (Quick Reference)

| Class | Namespace | File | Responsibility |
|-------|-----------|------|----------------|
| `Application` | `core` | `core/Application` | GLFW window + GL context + ImGui lifetime + main loop |
| `Clock` | `core` | `core/Clock` | Formatted local time string, called every frame |
| `Texture` | `core` | `core/Texture` | stb_image → OpenGL texture upload; failure diagnostics |
| `Theme` | `core` | `core/Theme` | One-time `applyTheme()` sets retro blue-dark ImGui palette |
| `Compositor` | `compositor` | `compositor/Compositor` | Pipeline: boot gate → Desktop → Windows → Taskbar |
| `Window` | `compositor` | `compositor/Window` | Abstract base: open/focus state + virtual `draw()` |
| `WindowManager` | `compositor` | `compositor/WindowManager` | Owns windows via `unique_ptr`; drives `draw()` calls |
| `BootSequence` | `shell` | `shell/BootSequence` | FSM: Bios→Splash→Loading→Done; skippable; blocks pipeline |
| `Desktop` | `shell` | `shell/Desktop` | Fullscreen wallpaper/gradient + clock + PWR confirm modal |
| `Taskbar` | `shell` | `shell/Taskbar` | Fixed bottom panel; toggle/focus apps; highlight on focus |
| `TaskManager` | `apps` | `apps/TaskManager` | Sortable fake process table, End Task, rolling perf graphs |
| `FileExplorerApp` | `apps` | `apps/FileExplorerApp` | Two-panel mock file browser with search filter |
| `SystemInfoApp` | `apps` | `apps/SystemInfoApp` | Animated system meters, fake network info, volume slider |

---

## 12. Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| **Immediate-mode UI (ImGui)** | Every widget is re-declared each frame — no persistent widget tree. Matches how real compositors redraw from scratch every frame. Keeps the code linear and readable. |
| **Static-method shell classes** | `Desktop` and `Taskbar` have no instance state — they read from ImGui's global IO and the app/window references passed in. Makes the call sites in Compositor obvious. |
| **Boot gate via early return** | `if (!boot_.isDone()) return` in Compositor prevents any desktop frame from ever rendering until boot completes. Zero coupling between BootSequence and the rest. |
| **`focusRequested_` two-frame flag** | ImGui's `SetNextWindowFocus()` must precede `Begin()`. The flag bridges the Taskbar (click detection) and the app window (Begin call) across one frame boundary. |
| **Ring buffer for graphs** | Fixed 90-element arrays with a write-head index. No allocations, no `memmove`, `PlotLines` understands the offset natively. |
| **`static` local for wallpaper** | C++ guarantees the initializer runs exactly once. Avoids a global and keeps the load co-located with the use site in `Desktop::draw()`. |
| **FetchContent for all deps** | Repo stays small; reproducible builds on any machine with CMake + a C++20 compiler and internet access. No manual library installation. |
| **Exclusive fullscreen blocked** | Window created with `nullptr` monitor argument (borderless windowed at native resolution) so OBS and the Windows compositor can capture every frame reliably. |
