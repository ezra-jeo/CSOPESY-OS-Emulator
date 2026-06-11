# CSOPESY OS Emulator — Development Plan (Phases 1–7)

## Context

The repo already contains a working Phase-0 scaffold: GLFW + OpenGL 3.3 + Dear ImGui
v1.91.6 (FetchContent), a per-frame Compositor pipeline (Desktop → Windows → Taskbar),
a live clock, a PWR shutdown button, and fully functional Task Manager, File Explorer,
and System Info windows. All three rubric components are visually present.

The remaining work is **completion + polish**: a real boot sequence, image-texture
wallpaper, Task Manager interactivity (sort / End Task / live graphs), window focus UX,
theming/fonts, and submission deliverables.

---

## Phase 1 — Image wallpaper + PWR confirm · `feat/desktop-wallpaper` ✅

*Component 1 polish. No dependencies.*

**Files:** `CMakeLists.txt`, `src/core/Texture.{h,cpp}`, `src/shell/Desktop.cpp`,
`assets/wallpapers/wallpaper.png` (optional).

**What was done:**
- Added `stb_image` via FetchContent (pinned SHA, header-only; `FetchContent_Populate`
  not `MakeAvailable` because stb has no CMakeLists.txt).
- New `core::Texture { unsigned id; int w,h; bool valid(); }` + `core::loadTexture(path)`:
  `stbi_load` → `glTexImage2D` (GL_RGBA, GL_LINEAR, GL_CLAMP_TO_EDGE) → `stbi_image_free`.
  `#define STB_IMAGE_IMPLEMENTATION` lives only in `Texture.cpp`.
- `Desktop::draw` caches via function-local static `core::Texture`. If `valid()`,
  `dl->AddImage((ImTextureID)(uintptr_t)id, p0, p1)`; else keeps gradient fallback.
  Note: ImGui v1.91.6 `ImTextureID` is `uint64_t`, not `void*`.
- PWR button lives in Taskbar only (Desktop uses `NoInputs`; buttons there never fire).
- Taskbar PWR opens `BeginPopupModal` → "Shut down CSOPESY?" → confirm calls
  `app.requestQuit()`.

**Exit criteria:** image wallpaper renders when the PNG exists; gradient shows when
absent; PWR asks before quitting.

---

## Phase 2 — Delta-time + Boot sequence · `feat/boot-sequence` ✅

*Full BIOS→splash→loading→desktop.*

**Files:** `src/core/Application.cpp`, `src/compositor/Compositor.{h,cpp}`,
`src/shell/BootSequence.{h,cpp}`.

**What was done:**
- `Application::run()` computes `float dt` from `glfwGetTime()` deltas.
- `Compositor::render()` → `render(float dt)`; `Application` passes `dt`.
- `Compositor` owns `shell::BootSequence boot_` by value.
  `render(dt)` early-return: `if (!boot_.isDone()) { boot_.update(dt); boot_.draw(); return; }`.
- `BootSequence`: state machine `Bios(1.5s)→Splash(1.5s)→Loading(2.0s)→Done`.
  Skip on `ImGui::IsKeyPressed(Space/Enter/Escape)` or `io.MouseClicked[0]`.
  Each state draws a fullscreen black ImGui window via `ImDrawList`:
  - **Bios:** "CSOPESY Megatrends" header, memory test, drive detection lines.
  - **Splash:** ASCII-art "CSOPESY" banner, version string.
  - **Loading:** "Loading CSOPESY OS…" + animated progress bar with percentage.

**Exit criteria:** launching plays BIOS → splash → loading → desktop; any key/click
skips; after boot the app behaves exactly as before.

---

## Phase 3 — Task Manager interactivity · `feat/taskmanager-table` ✅

*Component 3 polish. No hard dependency.*

**Files:** `src/apps/TaskManager.{h,cpp}`.

**What was done:**
- **Sortable:** `ImGuiTableFlags_Sortable` + `DefaultSort` on Process column.
  After `TableHeadersRow()`, reads `TableGetSortSpecs()`; if `SpecsDirty`,
  `std::sort` on `ColumnUserID`/`SortDirection`; clears `SpecsDirty`. Sort invalidates
  `selectedRow_`.
- **End Task:** index-based row loop; name cell is `Selectable(...SpanAllColumns)`
  that sets `int selectedRow_`. After `EndTable()`, `BeginDisabled` "End Task" button
  erases the row and resets `selectedRow_ = -1`. Erase happens after the loop.
- **Performance tab:** rolling buffers `float cpuHist_[90]/memHist_[90]; int histOffset_;
  float plotAccum_; float simTime_`. Sampled at ~10 Hz (`plotAccum_ += dt; if >= 0.1f`).
  Values simulated with `std::sin(simTime_)` for smooth variation. Rendered with
  `ImGui::PlotLines(..., values_offset=histOffset_)` for scrolling graphs.

**Exit criteria:** columns sort on header click; selecting + End Task removes the row;
Performance tab shows live scrolling CPU/Memory graphs.

---

## Phase 4 — Window focus & taskbar active-state · `feat/window-focus` ✅

*Cross-cutting UX.*

**Files:** `src/compositor/Window.h`, `src/apps/*.cpp`, `src/shell/Taskbar.cpp`.

**What was done:**
- Added to `Window` base: `bool focused_`, `bool focusRequested_`,
  `requestFocus(){ open_=true; focusRequested_=true; }`, `isFocused()`.
- Each app `draw()`: `if (focusRequested_){ ImGui::SetNextWindowFocus(); focusRequested_=false; }`
  before `Begin`; `focused_ = ImGui::IsWindowFocused(RootAndChildWindows)` after `Begin`;
  `focused_=false` in the collapsed/closed early-return branch.
- Taskbar uses a local `appButton()` helper that calls `win.requestFocus()` on click and
  pushes a brighter `ImGuiCol_Button` when `win.isFocused() && win.isOpen()`.

**Exit criteria:** clicking a taskbar button raises + focuses its window; the taskbar
button of the focused app is visibly highlighted.

---

## Phase 5 — App-screen polish · `feat/app-screens` ✅

*The two unique UI screens. No dependencies.*

**Files:** `src/apps/FileExplorerApp.cpp`, `src/apps/SystemInfoApp.cpp`.

**What was done:**
- **File Explorer:** `ciContains(haystack, needle)` case-insensitive substring filter
  wired to `searchBuf_`; file list rebuilt each frame showing only matches; status bar
  shows visible count or selected filename; selecting a file that no longer matches
  search is handled gracefully.
- **System Info:** `static float t` accumulates `io.DeltaTime`; CPU and Memory progress
  bars use `std::sin(t * …)` for gentle live animation; labels show computed values.

**Exit criteria:** typing in File Explorer filters files; both screens look intentional
and non-placeholder.

---

## Phase 6 — Theme + fonts · `style/theme-fonts` ✅

*Visual identity.*

**Files:** `CMakeLists.txt`, `src/core/Theme.{h,cpp}`, `src/core/Application.cpp`,
`assets/fonts/LiberationSans.ttf`.

**What was done:**
- `core::applyTheme()` sets `ImGuiStyle` rounding/padding and key `ImGuiCol_*`
  (WindowBg, TitleBgActive, Header, Button, Tab, Table, etc.) for a retro dark-blue
  OS look. Called in `initImGui()` right after `StyleColorsDark()`.
- `LiberationSans.ttf` committed from system fonts; loaded with
  `io.Fonts->AddFontFromFileTTF("assets/fonts/LiberationSans.ttf", 15.0f)` before
  `ImGui_ImplOpenGL3_Init`. Falls back to ImGui default if file absent.

**Exit criteria:** consistent themed look and a custom font across the whole shell.

---

## Phase 7 — Docs & submission · `docs/submission`

*Wrap-up.*

**Files:** `docs/DEVELOPMENT_PLAN.md` (this file), `docs/ARCHITECTURE.md`,
`docs/images/`, `README.md`.

**Tasks remaining:**
- Update `docs/ARCHITECTURE.md` to include the boot-stage pipeline diagram.
- Capture screenshots into `docs/images/`.
- Record the seamless video: launch → BIOS/splash → desktop → taskbar → all three
  windows → Task Manager sort/End-Task/Performance → window focus highlight → PWR
  confirm closes the app. No cuts, no force-quit. 480–720p, ≤ 1 GB.
- Build the PPTX (Cover, Video Walkthrough, Architecture Diagram, Code Snippets,
  written design discussion) with the MP4 embedded.

**Exit criteria:** video + PPTX meet every submission rule; app is only ever closed
via PWR.

---

## Dependencies & parallelization

```
Phase 1  feat/desktop-wallpaper  ─┐ (independent)
Phase 3  feat/taskmanager-table  ─┤ (independent)
Phase 5  feat/app-screens        ─┘ (independent)  → ran in parallel

Phase 2  feat/boot-sequence       → introduces render(float dt); land first
Phase 4  feat/window-focus        → touches Taskbar.cpp
Phase 6  style/theme-fonts        → also touches Taskbar.cpp; after Phase 4
Phase 7  docs/submission          → last; needs everything visually final
```

## New files introduced

- `src/core/Texture.{h,cpp}` (Phase 1)
- `src/core/Theme.{h,cpp}` (Phase 6)
- `assets/fonts/LiberationSans.ttf` (Phase 6)
- `assets/wallpapers/wallpaper.png` (optional, Phase 1)

## API change

- `Compositor::render()` → `Compositor::render(float dt)` (Phase 2) — the only
  signature change; everything else is additive.

## Verification checklist

1. **Build:** `cmake -S . -B build && cmake --build build --parallel` — clean after
   each phase. First configure fetches GLFW, ImGui, and stb (needs network).
2. **Run:** `./build/csopesy` from the repo root (assets/ is copied next to the binary
   by the POST_BUILD command).
3. **Smoke tests per phase:**
   - Phase 2: boot plays → skippable; after boot, normal desktop.
   - Phase 3: column headers sort; select row + End Task removes it; Performance tab
     shows scrolling graphs.
   - Phase 4: clicking a taskbar button focuses and highlights it.
   - Phase 5: typing in search box filters File Explorer; System Info bars animate.
4. **End-to-end before submission:** launch → boot → desktop (wallpaper + clock) →
   taskbar opens all three windows → Task Manager sort/End-Task/Performance → focus
   highlight → PWR confirm closes the app. No force-quit at any point.
