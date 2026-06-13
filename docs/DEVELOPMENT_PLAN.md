# CSOPESY OS Emulator — Development Plan

## Status summary

| Phase | Status |
|-------|--------|
| 0 — Scaffold | ✅ Done |
| Code review fixes (A1–A4, D1–D2, N1–N2) | ✅ Done |
| Bug fixes (vol slider, search, per-dir files) | ✅ Done |
| 1 — Wallpaper + PWR modal | ✅ Done |
| 2 — Boot sequence | ✅ Done |
| 3 — Task Manager interactivity | ✅ Done |
| 4 — Window focus & taskbar highlight | ✅ Done |
| 5 — App screen polish | ✅ Done |
| 6 — Theme + fonts | ✅ Done |
| 7 — Docs & submission | ✅ Done |

---

## Phase 0 — Scaffold ✅

- Full folder structure, CMake + FetchContent (GLFW 3.4, ImGui v1.91.6)
- `Application` GLFW/ImGui init/loop/shutdown
- All component classes as compiling stubs
- Live clock, PWR button, three taskbar buttons

---

## Phase 1 — Desktop Wallpaper + PWR Confirm ✅

**Files:** `CMakeLists.txt`, `src/core/Texture.{h,cpp}`, `src/shell/Desktop.cpp`

- stb_image via FetchContent; `core::Texture` helper loads PNG at first draw
- Gradient fallback when `assets/wallpapers/wallpaper.png` is absent
- PWR button opens `BeginPopupModal` → Confirm/Cancel before `requestQuit()`

---

## Phase 2 — Delta-time + Boot Sequence ✅

**Files:** `src/core/Application.cpp`, `src/compositor/Compositor.{h,cpp}`, `src/shell/BootSequence.{h,cpp}`

- `Application::run()` computes `float dt` from `glfwGetTime()` deltas
- `Compositor::render(float dt)` gates boot before main layers
- `BootSequence`: Bios(1.8 s) → Splash(1.8 s) → Loading(2.0 s) → Done
- Any key press or mouse click skips immediately to desktop

---

## Phase 3 — Task Manager Interactivity ✅

**Files:** `src/apps/TaskManager.{h,cpp}`

- `ImGuiTableFlags_Sortable` — click column header to sort asc/desc
- Index-based row loop; "End Task" button erases selected row
- Performance tab: 90-sample rolling CPU/Memory `PlotLines` at 10 Hz

---

## Phase 4 — Window Focus & Taskbar Active-State ✅

**Files:** `src/compositor/Window.h`, `src/apps/*.cpp`, `src/shell/Taskbar.cpp`

- `Window` base gains `focused_`, `focusRequested_`, `requestFocus()`, `isFocused()`
- Taskbar button color brightens when its window is the focused window
- Clicking a taskbar button for an open window raises + focuses it

---

## Phase 5 — App Screen Polish ✅

**Files:** `src/apps/SystemInfoApp.cpp`, `src/apps/FileExplorerApp.cpp`

- SystemInfoApp: CPU and Memory bars animate via `sinf(GetTime())` so the screen feels live
- FileExplorer: search bar filters per-directory file list (case-insensitive substring)

---

## Phase 6 — Theme + Fonts ✅

**Files:** `CMakeLists.txt`, `src/core/Theme.{h,cpp}`, `src/core/Application.cpp`, `assets/fonts/LiberationMono-Bold.ttf`

- `core::applyTheme()` — retro blue-dark palette, rounded corners, consistent spacing
- LiberationMono-Bold 15 pt loaded from `assets/fonts/` at startup

---

## Phase 7 — Docs & Submission ✅

**Files:** `docs/ARCHITECTURE.md`, `docs/DEVELOPMENT_PLAN.md`

- Architecture diagram updated to include boot stage and all new classes
- This plan updated with all phase statuses

---

## Submission checklist

- [ ] Record demo video: launch → boot plays → skip or wait → desktop → open all 3 windows → sort Task Manager → End Task → Performance tab → search in File Explorer → System Info animated bars → PWR → confirm → exit
- [ ] Build on the grader's machine: `cmake -S . -B build && cmake --build build --parallel && ./build/csopesy`
- [ ] Verify **no force-quit** — only PWR button exits
