# CSOPESY OS Emulator — Component Reference

A complete guide to what every component does, why it exists, and how it works technically.

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [How a Frame is Drawn](#2-how-a-frame-is-drawn)
3. [Core Layer](#3-core-layer)
   - Application
   - Clock
   - Texture
   - Theme
4. [Compositor Layer](#4-compositor-layer)
   - Compositor
   - Window
   - WindowManager
5. [Shell Layer](#5-shell-layer)
   - Desktop
   - Taskbar
   - BootSequence
6. [Apps Layer](#6-apps-layer)
   - TaskManager
   - FileExplorerApp
   - SystemInfoApp
7. [How the Layers Connect](#7-how-the-layers-connect)
8. [Key Design Patterns](#8-key-design-patterns)
9. [Quick-Change Cheat Sheet](#9-quick-change-cheat-sheet)
10. [Self-Help Resources](#10-self-help-resources)

---

## 1. Project Overview

CSOPESY is a desktop OS emulator built entirely in C++20 using three libraries:

| Library | Role |
|---------|------|
| **GLFW** | Creates the OS window, OpenGL context, and receives keyboard/mouse input |
| **OpenGL 3.3** | GPU rendering — draws textured quads, coloured rectangles, and text bitmaps |
| **Dear ImGui** | Immediate-mode GUI toolkit — every button, window, table, and graph is an ImGui widget |

There is **no game engine**, **no scene graph**, and **no retained widget tree**. Every single pixel is re-issued from scratch 60 times per second. ImGui is the entire UI stack.

---

## 2. How a Frame is Drawn

Every iteration of the main loop in `Application::run()` does exactly this:

```
1. glfwPollEvents()              — process keyboard, mouse, resize from the OS
2. ImGui_ImplOpenGL3_NewFrame()  — tell ImGui a new frame is starting
3. ImGui_ImplGlfw_NewFrame()     — feed window size and input state into ImGui
4. ImGui::NewFrame()             — ImGui resets its draw command buffer

5. compositor.render(dt)         — YOUR CODE: issue all draw commands (see below)

6. ImGui::Render()               — ImGui compiles draw commands into a GPU-ready list
7. glClear()                     — wipe the framebuffer
8. ImGui_ImplOpenGL3_RenderDrawData() — send the compiled list to the GPU
9. glfwSwapBuffers()             — flip front/back buffer → frame appears on screen
```

Step 5 is where all application logic lives. Everything you see is a draw command added to ImGui's internal `ImDrawList` during that step.

---

## 3. Core Layer

### `Application` — `src/core/Application.h/.cpp`

**Purpose:** The root object. Owns the GLFW window, the OpenGL context, and the ImGui context. Runs the main loop.

**Technical details:**

- Queries `glfwGetPrimaryMonitor()` at startup to get the native screen resolution, then creates a true fullscreen GLFW window at that resolution.
- Registers a GLFW close callback that always calls `glfwSetWindowShouldClose(w, GLFW_FALSE)` — this is how Alt+F4 is silently rejected. The window can only close when `running_` is set to `false` via `requestQuit()`.
- `initImGui()` loads `LiberationMono-Bold.ttf` at 15pt with a custom glyph range covering Basic Latin (U+0020–U+00FF) plus box-drawing and block-element characters (U+2500–U+259F) so the boot logo renders correctly.
- Delta time is computed each frame as `float dt = (float)(now - lastTime)` using `glfwGetTime()` (a high-resolution monotonic clock). This `dt` is passed to `Compositor::render(dt)` and flows into the boot sequence timer and performance graph sampler.

---

### `Clock` — `src/core/Clock.h/.cpp`

**Purpose:** Provides the formatted live clock string shown in the top-right corner of the desktop and the taskbar.

**Technical details:**

- Calls `std::time(nullptr)` and `std::localtime()` each frame.
- Formats with `strftime` into the pattern `"Weekday, Mon DD, YYYY | HH:MM AM/PM"`.
- Returns a `std::string` by value — cheap enough at 60 Hz.

---

### `Texture` — `src/core/Texture.h/.cpp`

**Purpose:** Loads an image file from disk and uploads it to the GPU as an OpenGL texture object.

**Technical details:**

- Uses **stb_image** (`stbi_load`) to decode the file into a raw RGBA byte array. Supports PNG, JPG, BMP, TGA, GIF.
- If loading fails, `stbi_failure_reason()` is printed to stderr so you can see whether the problem is a missing file, wrong format, or corrupt data.
- Calls the OpenGL texture upload pipeline:
  ```
  glGenTextures    → allocates a texture slot on the GPU, returns an integer ID
  glBindTexture    → makes that slot "active"
  glTexParameteri  → sets GL_LINEAR filtering and GL_CLAMP_TO_EDGE wrapping
  glTexImage2D     → uploads the raw pixel bytes to the GPU
  stbi_image_free  → frees the CPU-side copy (GPU has its own)
  ```
- Returns a `Texture { id, w, h }` struct. `id == 0` means loading failed (`valid()` returns false).
- The Desktop stores the texture in a `static` local variable — it is loaded exactly once on the first frame and never reloaded.

---

### `Theme` — `src/core/Theme.h/.cpp`

**Purpose:** Applies a retro navy/blue color palette to the entire ImGui style at startup.

**Technical details:**

- `applyTheme()` is called once in `Application::initImGui()`, after `ImGui::StyleColorsDark()`.
- Directly mutates `ImGui::GetStyle()` — sets `WindowRounding`, `FramePadding`, `ItemSpacing`, and 30+ `ImGuiCol_*` color slots.
- Because ImGui uses these values every frame to draw every widget, changing a single color here changes it everywhere in the app instantly.

---

## 4. Compositor Layer

### `Compositor` — `src/compositor/Compositor.h/.cpp`

**Purpose:** The render pipeline director. Decides what gets drawn each frame and in what order.

**Technical details:**

The entire pipeline is five lines:

```cpp
void Compositor::render(float dt) {
    if (!boot_.isDone()) {         // GATE: nothing draws until boot is done
        boot_.update(dt);
        boot_.draw();
        return;                    // early return — Desktop + apps never execute
    }
    shell::Desktop::draw(app_);    // Layer 1: fullscreen background
    wm_.drawWindows();             // Layer 2: floating app windows
    shell::Taskbar::draw(...);     // Layer 3: fixed taskbar on top
}
```

**Why this order matters:** ImGui draws everything into a single list in the order commands are issued. The Desktop must come first (it fills the whole screen). App windows go second (they float over the background). The Taskbar goes last — it calls `ImGui::BringWindowToDisplayFront()` internally, which moves it to the front of ImGui's window stack so it always appears above floating windows.

The `Compositor` also owns the three raw pointers to the app windows (`taskManager_`, `fileExplorer_`, `sysInfo_`). These are not `unique_ptr` — ownership stays with the `WindowManager`. The Compositor just holds non-owning pointers so it can pass them to `Taskbar::draw()`.

---

### `Window` — `src/compositor/Window.h`

**Purpose:** Abstract base class that every app window inherits from. Defines the shared lifecycle state.

**Technical details:**

```cpp
class Window {
protected:
    std::string title_;
    bool open_{ false };           // is the window currently shown?
    bool focused_{ false };        // does ImGui say this window has keyboard focus?
    bool focusRequested_{ false }; // was requestFocus() called this frame?
};
```

- `open_` starts as `false` — windows are closed until the user clicks their taskbar button.
- `focused_` is updated every frame inside each app's `draw()` by calling `ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)`. This is how the taskbar knows which button to highlight.
- `focusRequested_` is the one-frame signal mechanism: when the taskbar calls `requestFocus()`, the flag is set. On the very next frame, the app's `draw()` sees the flag, calls `ImGui::SetNextWindowFocus()` (which tells ImGui to focus this window before it opens), then clears the flag. This two-step is necessary because ImGui's focus API is "set before Begin, read after Begin."

---

### `WindowManager` — `src/compositor/WindowManager.h`

**Purpose:** Owns all app windows and drives their draw calls.

**Technical details:**

```cpp
template<typename T, typename... Args>
T* add(Args&&... args) {
    auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
    T* raw = ptr.get();
    windows_.push_back(std::move(ptr));
    return raw;
}
```

- Uses a **variadic template** so you can call `wm_.add<TaskManager>()` without knowing the constructor arguments at the call site — they're forwarded with perfect forwarding.
- Stores everything in `std::vector<std::unique_ptr<Window>>` — the vector owns the memory. The `raw` pointer returned is non-owning; it's safe as long as the WindowManager is alive (which is the entire duration of the program).
- `drawWindows()` iterates the vector and calls `draw()` on every window. Windows where `open_ == false` still call `draw()` — but their `draw()` implementations call `ImGui::Begin(..., &open_)`, and if `open_` is false ImGui skips the window body instantly.

---

## 5. Shell Layer

### `Desktop` — `src/shell/Desktop.h/.cpp`

**Purpose:** Draws the fullscreen desktop background, the live clock, and the PWR button with its shutdown confirmation dialog.

**Technical details:**

- Opens an ImGui window with `ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus`. The `NoBringToFrontOnFocus` flag is critical — without it, clicking the desktop would pull it in front of all the app windows.
- Gets access to `ImDrawList* dl = ImGui::GetWindowDrawList()`. The draw list is a low-level command buffer. `dl->AddImage(...)` and `dl->AddRectFilledMultiColor(...)` bypass ImGui's widget system entirely and go straight to GPU draw calls. This is used for the wallpaper and the gradient because widgets would add padding and borders.
- The wallpaper texture is stored in a `static` local: `static core::Texture wallpaper = core::loadTexture(...)`. The `static` keyword means this line executes only once — on the first call to `Desktop::draw()`. Every subsequent frame it reuses the same GPU texture ID.
- The PWR button uses `ImGui::OpenPopup("##pwr_confirm")` — this doesn't open a window immediately, it sets a deferred flag. On the same frame, `ImGui::BeginPopupModal("##pwr_confirm", ...)` checks that flag and, if set, opens a centered blocking modal.

---

### `Taskbar` — `src/shell/Taskbar.h/.cpp`

**Purpose:** The fixed bottom panel with app launcher buttons, clock, and PWR.

**Technical details:**

- Positioned at `{0, H - 42}` with a fixed size of `{W, 42}` — it always snaps to the bottom regardless of window size.
- Calls `ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow())` immediately after `Begin()`. This is an `imgui_internal.h` function that moves the window to the top of ImGui's z-order stack so it draws over floating app windows.
- **Button highlighting** uses `PushStyleColor` / `PopStyleColor`: before each button, the three button color slots (`ImGuiCol_Button`, `ImGuiCol_ButtonHovered`, `ImGuiCol_ButtonActive`) are overridden with bright blue (if the app is focused) or dark blue (if not). `PopStyleColor(3)` restores them immediately after the button. This is purely visual — no logic changes.
- **Button click logic:**
  ```cpp
  fileExp.isOpen() ? fileExp.requestFocus() : fileExp.toggle();
  ```
  If the window is already open: bring it to the front (`requestFocus()`). If it's closed: open it (`toggle()` flips `open_` to true). There is no "minimize to taskbar" — closing a window just hides it; reopening re-creates it from the same in-memory state.
- The right-side clock is right-aligned by computing: `rightX = W - clockWidth - pwrButtonWidth - 24px` and calling `SameLine(rightX)`.

---

### `BootSequence` — `src/shell/BootSequence.h/.cpp`

**Purpose:** The startup animation that plays before the desktop appears. A simple finite state machine (FSM).

**Technical details:**

States: `Bios(1.8s) → Splash(1.8s) → Loading(2.0s) → Done`

```
update(dt) is called every frame:
  timer_ += dt
  if timer_ >= duration_for_current_state:
      advance to next state, reset timer_ = 0
  if any key or mouse click is detected:
      skip directly to Done
```

- Uses `ImGui::IsKeyPressed(ImGuiKey_Space)` and `io.MouseClicked[0]` to detect the skip input — these are ImGui's input polling functions, not GLFW callbacks.
- `draw()` is a separate method. Each state renders its own fullscreen ImGui window with `ImGuiWindowFlags_NoInputs` (so it doesn't steal mouse events from the skip detection) and `ImGuiWindowFlags_NoBringToFrontOnFocus`.
- The Bios screen uses `ImGui::SetCursorPos()` to manually place each text line at exact pixel coordinates — more reliable than letting ImGui auto-layout text in a boot-screen context.
- The Splash screen uses `ImGui::CalcTextSize()` to measure each text string's width, then `(W - width) * 0.5f` to center it horizontally.
- The Loading screen draws the progress bar directly via `ImDrawList::AddRectFilled` (for the filled portion) and `ImDrawList::AddRect` (for the outline border), bypassing `ImGui::ProgressBar()` for more visual control.
- Once `isDone()` returns `true`, both `update()` and `draw()` return immediately without doing anything. The Compositor never calls them again because the early-return branch is guarded by `!boot_.isDone()`.

---

## 6. Apps Layer

### `TaskManager` — `src/apps/TaskManager.h/.cpp`

**Purpose:** Simulates Windows Task Manager with a process list and performance graphs.

**Technical details:**

**Process table (Processes tab):**
- Uses `ImGui::BeginTable` with `ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg`.
- `ScrollY` enables a scrollable region with a fixed height (290px) — without this, adding many rows would overflow the window.
- `RowBg` alternates row background colors automatically.
- After `TableHeadersRow()`, `ImGui::TableGetSortSpecs()` returns a pointer to the current sort state. If `SpecsDirty` is true (the user just clicked a column header), a `std::sort` is run on `processes_` using a lambda that switches on the column index. `SpecsDirty` is then cleared to prevent re-sorting every frame.
- Row selection uses `ImGuiSelectableFlags_SpanAllColumns` so clicking anywhere on the row (not just the name cell) selects it.
- "End Task" calls `processes_.erase(processes_.begin() + selectedRow_)` — a vector erase that shifts all subsequent elements down. `selectedRow_` is reset to -1.

**Performance graphs (Performance tab):**
- Two circular buffers: `float cpuHist_[90]` and `float memHist_[90]`, with `histOffset_` as the write head.
- Sampled at ~10 Hz using an accumulator: `plotAccum_ += io.DeltaTime`. When `plotAccum_ >= 0.1f`, a new sample is written and `histOffset_ = (histOffset_ + 1) % 90`.
- `ImGui::PlotLines("##cpu", cpuHist_, 90, histOffset_, ...)` takes the ring buffer, the count, and the current write offset — it knows to start reading from `histOffset_` and wrap around, displaying the 9-second history correctly.
- CPU value = sum of all `p.cpu` values in `processes_`, clamped to 100.
- Memory value = `55 + 8 * sin(time * 0.4)` — purely synthetic oscillation.

---

### `FileExplorerApp` — `src/apps/FileExplorerApp.h/.cpp`

**Purpose:** A mock two-panel file browser.

**Technical details:**

- **Data structure:** Two static arrays — `kDirs[6]` (directory names) and `kDirFiles[6][8]` (up to 8 file names per directory). A third array `kDirFileCounts[6]` says how many valid entries each directory has. The `[8]` bound is generous padding; unused slots hold `nullptr`.
- **Two-panel layout:** Uses `ImGui::BeginChild("##dirs", {140, panelH}, true)` and `ImGui::BeginChild("##files", {0, panelH}, true)` with `ImGui::SameLine()` between them. `{0, panelH}` means "take all remaining horizontal space." Each child window is an independently scrollable region with its own clipping rectangle.
- **Search filter:** A `containsCI` lambda performs case-insensitive substring search using a manual double-pointer walk with `std::tolower`. This runs on every file every frame — fast enough for a small static list.
- **Selection:** `selectedDir_` and `selectedFile_` are plain `int` indices. They are reset to `-1` whenever the directory changes to prevent stale selections.

---

### `SystemInfoApp` — `src/apps/SystemInfoApp.h/.cpp`

**Purpose:** Displays fake hardware statistics with animated progress bars.

**Technical details:**

- The three progress bars (CPU, Memory, Disk) use `sinf(ImGui::GetTime() * speed) * amplitude + base` to compute their current value each frame. `ImGui::GetTime()` returns the number of seconds since ImGui was initialised — a monotonic clock. The result makes the bars gently oscillate, simulating live system load.
- `ImGui::ProgressBar(fraction, size, overlay_text)` draws a bar from 0.0 to 1.0 with an optional text label centered over it.
- The volume slider stores its value in a member variable `vol_` (0–100). `ImGui::SliderFloat` mutates it in place — the slider widget both reads and writes the variable every frame.

---

## 7. How the Layers Connect

```
┌─────────────────────────────────────────────────────────────┐
│  Application  (owns GLFW window + ImGui context)            │
│                                                             │
│  Main loop ──► compositor.render(dt)                        │
│                    │                                        │
│              ┌─────▼──────────────────────────────────┐     │
│              │  Compositor                            │     │
│              │                                        │     │
│              │  if !boot.isDone():                    │     │
│              │      boot.update(dt) ──► boot.draw()  │     │
│              │      return  ◄── nothing else runs     │     │
│              │                                        │     │
│              │  Desktop::draw()     ← Layer 1         │     │
│              │  wm_.drawWindows()   ← Layer 2         │     │
│              │    ├─ TaskManager::draw()               │     │
│              │    ├─ FileExplorerApp::draw()           │     │
│              │    └─ SystemInfoApp::draw()             │     │
│              │  Taskbar::draw()     ← Layer 3 (top)   │     │
│              └────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

**The focus handshake between Taskbar and Window:**

```
Frame N:   User clicks [F] Files button in Taskbar
           → fileExp.isOpen() is true
           → fileExp.requestFocus() is called
           → sets focusRequested_ = true, open_ = true

Frame N+1: Compositor calls wm_.drawWindows()
           → FileExplorerApp::draw() runs
           → sees focusRequested_ == true
           → calls ImGui::SetNextWindowFocus()   ← must happen BEFORE Begin()
           → clears focusRequested_ = false
           → ImGui::Begin() opens the window with focus
           → ImGui::IsWindowFocused() returns true
           → sets focused_ = true

Every frame: Taskbar reads fileExp.isFocused()
             → returns true → button renders in bright blue
```

---

## 8. Key Design Patterns

**Immediate-mode UI**
There is no "create a button" and "register a click handler." Every frame you say "draw a button here" and immediately check if it returned `true` (was clicked this frame). Widget state (hover, press) is managed entirely by ImGui internally.

**Static local for one-time init**
```cpp
static core::Texture wallpaper = core::loadTexture("assets/wallpapers/wallpaper.png");
```
The C++ `static` inside a function means "run this line once, the first time this function is called." Used for the wallpaper load — the GPU texture is created once and reused every frame.

**PushStyleColor / PopStyleColor**
ImGui has a global style stack. You push overrides before a widget and pop them after. This is how the taskbar buttons change color based on focus state without any conditional branching in the widget rendering code itself.

**Ring buffer for graphs**
The performance graphs use a fixed-size array with a write-head index that wraps with `% 90`. This avoids allocations and `memmove` while automatically dropping the oldest sample. `PlotLines` is handed the offset so it knows where the "oldest" sample is.

**FetchContent for dependencies**
CMake's `FetchContent` downloads GLFW, ImGui, and stb at configure time. You never manually install libraries or manage DLLs — CMake handles it. The first configure takes ~30 seconds to clone; subsequent configures are instant (cached in `build/_deps/`).

---

## 9. Quick-Change Cheat Sheet

| Goal | File | What to change |
|------|------|----------------|
| Change UI colors globally | `src/core/Theme.cpp` | `ImGui::PushStyleColor` values; use `ImGui::ShowStyleEditor()` at runtime to preview |
| Change font or size | `src/core/Application.cpp:66` | TTF path or `15.0f` size value |
| Add a wallpaper | Drop `wallpaper.png` in `assets/wallpapers/` | File auto-loads on next build/run |
| Change boot duration | `src/shell/BootSequence.cpp:7–9` | `kBiosDur`, `kSplashDur`, `kLoadDur` constants |
| Add processes to Task Manager | `src/apps/TaskManager.cpp:10–24` | Push more `ProcessRow` structs in the constructor |
| Add directories/files in Explorer | `src/apps/FileExplorerApp.cpp:7–16` | Extend `kDirs`, `kDirFiles`, `kDirFileCounts` |
| Add a new app window | New file in `src/apps/` | Extend `compositor::Window`, add to `Compositor.cpp` + `Taskbar.cpp` |
| Right-align any text | Anywhere | `ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(text).x);` |
| Scale text larger temporarily | Anywhere | `ImGui::SetWindowFontScale(2.0f); /* text */ ImGui::SetWindowFontScale(1.0f);` |
| Diagnose wallpaper not loading | Check stderr | Run from terminal; error message shows exact `stbi_failure_reason()` |

---

## 10. Self-Help Resources

| Topic | Resource |
|-------|----------|
| **ImGui — every widget with live code** | `build/_deps/imgui-src/imgui_demo.cpp` (run or read) |
| **ImGui — colors and style tuning** | Call `ImGui::ShowStyleEditor()` anywhere in the app, tweak live, copy to `Theme.cpp` |
| **ImGui — fonts** | `build/_deps/imgui-src/docs/FONTS.md` |
| **ImGui — full wiki** | https://github.com/ocornut/imgui/wiki |
| **GLFW — input and window** | https://www.glfw.org/docs/latest/ |
| **OpenGL — textures explained** | https://learnopengl.com/Getting-started/Textures |
| **OpenGL — reference pages** | https://docs.gl |
| **stb_image — all docs in header** | `build/_deps/stb-src/stb_image.h` (open and read the top comments) |
| **C++ standard library** | https://en.cppreference.com |
| **`std::filesystem` for real file browsing** | https://en.cppreference.com/w/cpp/filesystem |
