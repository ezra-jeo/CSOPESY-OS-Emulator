# CSOPESY OS Emulator — Development Plan

## Phases

### Phase 0 — Scaffold (DONE)
- Full folder structure, CMake + FetchContent, `.gitignore`
- `Application` with GLFW+ImGui init/loop/shutdown
- All component classes exist as compiling stubs
- Live clock visible in corner; PWR button closes the app
- Three taskbar buttons toggle their respective (empty) windows

**Exit criteria:** `cmake -S . -B build && cmake --build build` succeeds; window opens with clock + PWR.

---

### Phase 1 — Desktop (Component 1)
- Replace placeholder background with a real gradient drawn via `ImDrawList`
  (`AddRectFilledMultiColor`) or load an XP-Bliss texture with `stb_image`
- Polish corner clock (font, color, shadow)
- PWR button: confirmation modal before exit

**Exit criteria:** Component 1 fully satisfies rubric checklist.

---

### Phase 2 — Taskbar (Component 2 chrome)
- Fixed bottom panel with correct height and styling
- Icon buttons: folder icon (File Explorer), monitor icon (System Info), chart icon (Task Manager)
- Right-side cluster: VOL / NET / PWR icons + live time chip
- Hover / active / tooltip states on all buttons

**Exit criteria:** Taskbar looks polished; each button toggles the correct window.

---

### Phase 3 — Task Manager (Component 3)
- `ImGui::BeginTable` with columns: Process, CPU %, Memory (MB)
- 10–15 dummy rows with realistic-looking values
- Column headers sortable (ImGui sort specs)
- "End Task" button per row (no-op or removes row for effect)

**Exit criteria:** Task Manager window matches rubric screenshot.

---

### Phase 4 — Two App Screens
- **FileExplorerApp:** folder tree on left, file list on right, search bar at top
- **SystemInfoApp:** OS version card, CPU/RAM bars, network status panel
- Both windows are draggable, resizable, closeable

**Exit criteria:** Two visually distinct, non-trivial UI screens are functional.

---

### Phase 5 — Boot Sequence (optional polish)
- `BootSequence` state machine: BIOS text crawl → CSOPESY ASCII splash → loading bar → desktop
- Custom monospace/pixel font loaded from `assets/fonts/`
- Boot can be skipped with any key

**Exit criteria:** Full boot-to-desktop sequence plays on startup.

---

### Phase 6 — Polish & Submission
- ImGui theme pass (colors, rounding, padding)
- Window drag/focus/z-order niceties via `WindowManager`
- Record seamless video: press Run/Debug → demo all components → PWR to exit
- Build PPTX with embedded MP4, Architectural Diagram, code snippets

---

## Suggested Task Division

| Member | Ownership |
|--------|-----------|
| A | Core / Compositor / CMake / `Application`, `Window`, `WindowManager` |
| B | Desktop wallpaper, clock, PWR, Taskbar styling (Phases 1–2) |
| C | Task Manager table, FileExplorer, SystemInfo layouts (Phases 3–4) |
| D | Boot sequence, theming, assets, video recording, PPT (Phases 5–6) |

Adjust to actual group size; pairs can co-own phases.

---

## Open Questions (deliberate before Phase 1)

1. **Wallpaper:** pure ImGui gradient (safest, zero assets) vs. bundled XP-Bliss image (needs `stb_image` + license note)?
2. **App screens:** keep File Explorer + System Info, or swap one for a Terminal mock / Settings panel?
3. **Boot sequence:** implement Phase 5, or go straight to desktop?
4. **Window chrome:** draggable & resizable (ImGui default) vs. fixed-position panels?
