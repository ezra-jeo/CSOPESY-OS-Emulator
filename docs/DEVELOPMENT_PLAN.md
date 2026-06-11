# CSOPESY OS Emulator — Development Plan

## Current state (Phase 0 ✅)

The scaffold on `main` is already further than a typical stub:

| Component | Status |
|-----------|--------|
| GLFW + OpenGL 3.3 + ImGui v1.91.6 init/loop/shutdown | ✅ Functional |
| Per-frame Compositor pipeline (Desktop → Windows → Taskbar) | ✅ Functional |
| Desktop: navy-blue gradient wallpaper, live clock, PWR button | ✅ Functional |
| Taskbar: fixed bottom panel, 3 toggle buttons, clock, PWR | ✅ Functional |
| Task Manager: 13-row dummy table, 4 columns, tab bar | ✅ Functional |
| File Explorer: two-panel dir/file list, search bar | ✅ Functional |
| System Info: OS card, CPU/Mem/Disk bars, network, volume | ✅ Functional |
| BootSequence: state enum + timer stub, **not wired** | ⚠️ Stub |
| Image wallpaper, font loading, theme | ❌ Not started |
| Task Manager sort / End Task / performance graphs | ❌ Not started |
| Window focus UX | ❌ Not started |
| Boot sequence animation | ❌ Not started |

"The rest" is completion + polish — no green-field components remain.

---

## Phases 1–7

Each phase maps to one branch. Each is self-contained and can be handed to an
agent independently. Dependencies are marked; independent phases can run in parallel.

---

### Phase 1 — Image wallpaper + PWR confirmation  ·  `feat/desktop-wallpaper`

**Goal:** Component 1 fully polished. No dependencies.

**Files changed:**
- `CMakeLists.txt` — add stb FetchContent + include dir
- `src/core/Texture.h` / `src/core/Texture.cpp` *(new — add to SOURCES)*
- `src/shell/Desktop.cpp`
- `assets/wallpapers/wallpaper.png` *(committed asset)*

**Approach:**

1. **stb_image via FetchContent** (consistent with existing GLFW/ImGui style). stb has no
   `CMakeLists.txt`, so `FetchContent_MakeAvailable` just downloads it; use
   `target_include_directories(csopesy PRIVATE ${stb_SOURCE_DIR})`. **Pin a commit SHA**
   (not `master`) for reproducibility.

2. **`core::Texture` helper** (`src/core/Texture.{h,cpp}`):
   ```cpp
   namespace core {
     struct Texture { unsigned id{0}; int w{0}, h{0}; bool valid() const { return id!=0; } };
     Texture loadTexture(const char* path);
   }
   ```
   `loadTexture`: `stbi_load(path,&w,&h,&ch,4)` → `glGenTextures` / `glTexImage2D`
   (GL_RGBA, GL_LINEAR, GL_CLAMP_TO_EDGE) → `stbi_image_free`. Returns empty `Texture{}`
   on failure (the fallback signal). `#define STB_IMAGE_IMPLEMENTATION` lives **only** in
   `Texture.cpp` — never in a header.

3. **Desktop wallpaper** — in `Desktop::draw`, cache via **function-local static**
   (`static core::Texture s_wp = core::loadTexture("assets/wallpapers/wallpaper.png")`),
   which loads exactly once on first call. Draw:
   ```cpp
   if (s_wp.valid())
       dl->AddImage((ImTextureID)(intptr_t)s_wp.id, p0, p1);
   else
       dl->AddRectFilledMultiColor(p0, p1, /* existing gradient */);
   ```
   > **Gotcha:** ImGui v1.91.6 has `ImTextureID = void*` — use `(void*)(intptr_t)id`.
   > The v1.92+ `ImU64` change does **not** apply here.

4. **PWR confirmation modal** — replace the direct `app.requestQuit()` call with
   `ImGui::OpenPopup("Shutdown?")`, then `BeginPopupModal` with "Confirm" / "Cancel"
   buttons.

**Exit criteria:** PNG wallpaper renders when the file exists; gradient shows as fallback
when it's absent; PWR opens a confirm dialog before quitting.

---

### Phase 2 — Delta-time + Boot sequence  ·  `feat/boot-sequence`

**Goal:** Full BIOS→splash→loading→desktop on launch, skippable by keypress.
*Land this before any phase that also touches `Application::run()` or `Compositor`.*

**Files changed:**
- `src/core/Application.cpp`
- `src/compositor/Compositor.h` / `Compositor.cpp`
- `src/shell/BootSequence.h` / `BootSequence.cpp`

**Approach:**

1. **Delta-time in `Application::run()`** (no `<chrono>` needed — GLFW has a timer):
   ```cpp
   double lastTime = glfwGetTime();
   // in loop, after glfwPollEvents():
   double nowTime = glfwGetTime();
   float dt = static_cast<float>(nowTime - lastTime);
   lastTime = nowTime;
   ```
   Change `compositor.render()` → `compositor.render(dt)`.

2. **`Compositor::render()` → `render(float dt)`** — the only API ripple in the codebase.
   Add `shell::BootSequence boot_` member (by value; include `shell/BootSequence.h` in
   `Compositor.h`). New render body:
   ```cpp
   if (!boot_.isDone()) {
       boot_.update(dt);
       boot_.draw();
       return;          // desktop/windows/taskbar are not drawn during boot
   }
   shell::Desktop::draw(app_);
   wm_.drawWindows();
   shell::Taskbar::draw(app_, *taskManager_, *fileExplorer_, *sysInfo_);
   ```

3. **`BootSequence` implementation** — change initial `state_` from `State::Done` to
   `State::Bios`. Phase timings: Bios ~1.5 s, Splash ~1.5 s, Loading ~2.0 s.
   `update(dt)` accumulates `timer_ += dt`; advances state when threshold exceeded;
   resets `timer_`. Loading phase: derive `float progress = timer_ / kLoadingDur` for a
   progress bar.

   **Skip on keypress** — at the top of `update()`:
   ```cpp
   ImGuiIO& io = ImGui::GetIO();
   if (ImGui::IsKeyPressed(ImGuiKey_Space) ||
       ImGui::IsKeyPressed(ImGuiKey_Enter) ||
       io.MouseClicked[0])
       state_ = State::Done;
   ```
   > **Gotcha:** use `ImGui::IsKeyPressed`, NOT `glfwGetKey`. The latter would require
   > passing `GLFWwindow*` down into shell code, breaking the layering. `IsKeyPressed`
   > reads global ImGui state and works regardless of per-window input flags.

   `draw()` renders a fullscreen ImGui window (no `NoInputs`) for each state:
   - **Bios:** dark screen, monospaced text crawl ("CSOPESY Megatrends BIOS v2.0…")
   - **Splash:** "CSOPESY" ASCII art block, version/team line
   - **Loading:** progress bar driven by `progress`, "Loading…" label

**Exit criteria:** launching plays BIOS → splash → loading → desktop; Space/Enter/click
skips to desktop immediately; after boot the app behaves identically to today.

---

### Phase 3 — Task Manager interactivity  ·  `feat/taskmanager-table`

**Goal:** Component 3 fully polished. No hard dependency.

**Files changed:**
- `src/apps/TaskManager.h` / `TaskManager.cpp`
- Add `#include <algorithm>`

**Approach:**

1. **Sortable columns** — add `ImGuiTableFlags_Sortable` to `tflags`. After
   `TableHeadersRow()` and **before** the row loop:
   ```cpp
   if (auto* specs = ImGui::TableGetSortSpecs())
       if (specs->SpecsDirty && processes_.size() > 1) {
           const auto& s = specs->Specs[0];
           bool asc = s.SortDirection == ImGuiSortDirection_Ascending;
           std::sort(processes_.begin(), processes_.end(),
               [&](const ProcessRow& a, const ProcessRow& b){
                   switch (s.ColumnIndex) {
                     case 0: return asc ? a.name<b.name : b.name<a.name;
                     case 1: return asc ? a.cpu<b.cpu   : b.cpu<a.cpu;
                     case 2: return asc ? a.memory<b.memory : b.memory<a.memory;
                     default:return asc ? a.status<b.status : b.status<a.status;
                   }});
           specs->SpecsDirty = false;
       }
   ```
   > **Gotcha:** sort the vector *before* the row-draw loop, never during iteration.
   > Always reset `SpecsDirty = false` or the sort runs every frame.

2. **End Task** — switch loop to index-based (`for (int i=0; ...)`). Make the name cell a
   `Selectable(..., ImGuiSelectableFlags_SpanAllColumns)` that sets `int selectedRow_`.
   After `EndTable()`:
   ```cpp
   ImGui::BeginDisabled(selectedRow_ < 0);
   if (ImGui::Button("End Task")) {
       processes_.erase(processes_.begin() + selectedRow_);
       selectedRow_ = -1;
   }
   ImGui::EndDisabled();
   ```
   > **Gotcha:** erase after the loop, reset index immediately to avoid dangling.

3. **Performance tab** — replace the placeholder with rolling-buffer graphs. Add members:
   `float cpuHist_[90]{}, memHist_[90]{}; int histOffset_{0}; float plotAccum_{0};`.
   Sample at ~10 Hz (gate with `plotAccum_ += GetIO().DeltaTime; if (plotAccum_>0.1f){...}`).
   Render with `ImGui::PlotLines("CPU %", cpuHist_, 90, histOffset_, ...)` — passing
   `histOffset_` makes ImGui render the ring buffer as a continuous scrolling graph.

**Exit criteria:** column headers sort on click; selecting a row + End Task removes it;
Performance tab shows live scrolling CPU/Memory history.

---

### Phase 4 — Window focus & taskbar active-state  ·  `feat/window-focus`

**Goal:** Clicking a taskbar button raises the window; the button of the focused app is
highlighted. *Sequence before Phase 6 (both touch Taskbar).*

**Files changed:**
- `src/compositor/Window.h`
- `src/apps/TaskManager.cpp`, `FileExplorerApp.cpp`, `SystemInfoApp.cpp`
- `src/shell/Taskbar.cpp`

**Approach:**

ImGui already handles click-to-front z-order and focus internally. **Do not reorder
`WindowManager`**. Only add the UX glue:

1. **`Window` base additions:**
   ```cpp
   bool focused_{false};
   bool focusRequested_{false};
   void requestFocus() { open_ = true; focusRequested_ = true; }
   bool isFocused() const { return focused_; }
   ```

2. **Each app's `draw()`** — before `Begin`:
   ```cpp
   if (focusRequested_) { ImGui::SetNextWindowFocus(); focusRequested_ = false; }
   ```
   After `Begin`:
   ```cpp
   focused_ = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
   ```
   In the collapsed/early-return branch: `focused_ = false`.

3. **`Taskbar::draw`** — replace `toggle()` calls with `requestFocus()` for already-open
   apps; push a brighter button color when `app.isFocused()`:
   ```cpp
   bool active = taskMgr.isFocused() && taskMgr.isOpen();
   if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f,0.55f,1.0f,1.0f));
   if (ImGui::Button(" [T] Tasks ")) taskMgr.requestFocus();
   if (active) ImGui::PopStyleColor();
   ```
   No signature change to `Taskbar::draw` — it already holds all three app refs.

**Exit criteria:** clicking a taskbar button for an open window raises it; that button is
visibly brighter than the others.

---

### Phase 5 — App-screen polish  ·  `feat/app-screens`

**Goal:** File Explorer and System Info look intentional for the video. No dependencies.

**Files changed:**
- `src/apps/FileExplorerApp.cpp`
- `src/apps/SystemInfoApp.cpp`

**Approach:**
- **File Explorer:** wire `searchBuf_` to actually filter the file list (case-insensitive
  `strstr`); improve selection visual feedback; add a breadcrumb-style path display.
- **System Info:** add subtle animation to the CPU/Mem bars using `sinf(GetTime())` so
  the readings feel live rather than frozen; ensure the layout reads like a real panel.

**Exit criteria:** typing in File Explorer narrows the file list; both screens look
distinct and non-placeholder for the walkthrough video.

---

### Phase 6 — Theme + fonts  ·  `style/theme-fonts`

**Goal:** Consistent visual identity across the whole shell.
*Sequence after Phase 4 (both touch Taskbar styling).*

**Files changed:**
- `CMakeLists.txt` — add `src/core/Theme.cpp` to SOURCES
- `src/core/Theme.h` / `src/core/Theme.cpp` *(new)*
- `src/core/Application.cpp`
- `assets/fonts/<font>.ttf` *(committed asset — must be present at runtime)*

**Approach:**

1. **`core::applyTheme()`** in `Theme.{h,cpp}`:
   ```cpp
   namespace core { void applyTheme(); }
   ```
   Body: `ImGuiStyle& s = ImGui::GetStyle();` + set `s.WindowRounding`, `s.FrameRounding`,
   `s.ItemSpacing`, and key `ImGuiCol_*` (WindowBg, TitleBgActive, Header, Button, etc.)
   for the retro-OS look.

2. **Call site:** `Application::initImGui()`, right after `ImGui::StyleColorsDark()`:
   ```cpp
   core::applyTheme();
   ```

3. **Font loading** — also in `initImGui()`, **before** `ImGui_ImplOpenGL3_Init`:
   ```cpp
   io.Fonts->AddFontFromFileTTF("assets/fonts/<font>.ttf", 16.0f);
   ```
   > **Gotcha:** fonts must be loaded before the first `NewFrame`. `AddFontFromFileTTF`
   > **asserts in debug** if the file is missing — the TTF must be committed to
   > `assets/fonts/` so the POST_BUILD `copy_directory` puts it next to the binary.

**Exit criteria:** a custom monospace/pixel font is used throughout; the shell has a
consistent color scheme; the old `StyleColorsDark` baseline is replaced/augmented.

---

### Phase 7 — Docs & submission  ·  `docs/submission`

**Goal:** All deliverables ready. Sequence last.

**Files changed:**
- `docs/ARCHITECTURE.md` — add boot-sequence stage to the pipeline diagram
- `docs/images/` — drop in screenshots for README/PPT
- `README.md` — update controls if Phase 2 added a skip key

**Approach:**
- Update the architecture diagram to include the `BootSequence` gate before the
  Desktop/Windows/Taskbar layers.
- Record the seamless video:
  - Press **Run/Debug** from IDE → BIOS text crawl → CSOPESY splash → loading bar →
    desktop with wallpaper + live clock → open all three windows via taskbar → demo Task
    Manager sort + End Task + Performance tab → demo File Explorer search → demo System
    Info panel → click **PWR** → confirm → app closes. **No cuts. 480–720p. ≤1 GB.**
  - Do **not** modify any code while recording.
- Build PPTX: Cover → Video Walkthrough (MP4 **embedded**, not linked) →
  Architectural Diagram → Code Snippets → Written design discussion. File size must
  reflect the embedded video.

**Exit criteria:** video + PPTX satisfy every submission rule; app is never force-quit.

---

## Suggested 4-member task split

| Member | Branch(es) | Phases |
|--------|-----------|--------|
| A | `feat/boot-sequence` | 2 (boot/core — the only API ripple) |
| B | `feat/desktop-wallpaper`, `feat/app-screens` | 1 + 5 |
| C | `feat/taskmanager-table` | 3 |
| D | `feat/window-focus`, `style/theme-fonts`, `docs/submission` | 4 + 6 + 7 |

Phases 1, 3, 5 have no dependencies and can start immediately in parallel.
Phase 2 introduces `render(float dt)` — land it before any other branch that touches
`Application::run()` or `Compositor`. Phase 6 sequences after Phase 4 (both touch
Taskbar). Phase 7 goes last.

---

## New files introduced across all phases

| File | Phase |
|------|-------|
| `src/core/Texture.{h,cpp}` | 1 |
| `src/core/Theme.{h,cpp}` | 6 |
| `assets/wallpapers/wallpaper.png` | 1 |
| `assets/fonts/<font>.ttf` | 6 |

## Only breaking API change

`Compositor::render()` → `Compositor::render(float dt)` (Phase 2). Everything else is
additive (new members, new files, new flags).
