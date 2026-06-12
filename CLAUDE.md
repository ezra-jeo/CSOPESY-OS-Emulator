# CLAUDE.md — CSOPESY OS Emulator

Guidelines for Claude Code and all contributors in this repository.

## Branch naming

Use conventional branch prefixes:

| Prefix | When to use |
|--------|-------------|
| `feat/<short-name>` | New component or feature (e.g. `feat/taskbar-icons`) |
| `fix/<short-name>` | Bug fix (e.g. `fix/pwr-button-crash`) |
| `chore/<short-name>` | Build, tooling, config, scaffolding (e.g. `chore/project-setup`) |
| `style/<short-name>` | Visual/theme changes with no logic change |
| `docs/<short-name>` | Documentation only |
| `refactor/<short-name>` | Code restructure with no behaviour change |

Never push directly to `main`. Always branch, then open a PR.

## Commit messages — Conventional Commits

Format: `<type>(<scope>): <short imperative summary>`

```
feat(taskbar): add icon buttons for file explorer and sys info
fix(desktop): correct PWR button flag name for imgui v1.91
chore(cmake): disable Wayland, require X11 only
docs(arch): add per-frame pipeline diagram to ARCHITECTURE.md
style(desktop): adjust gradient colours to navy-blue theme
refactor(compositor): extract WindowManager from Compositor
```

**Types:** `feat` · `fix` · `chore` · `docs` · `style` · `refactor` · `test` · `perf`

**Scopes** (use the folder/component name): `core` · `compositor` · `shell` · `desktop` · `taskbar` · `taskmanager` · `fileexplorer` · `sysinfo` · `boot` · `cmake` · `assets`

Keep the subject line ≤ 72 characters. Use the body for *why*, not *what*.

## Code style

- C++20, no exceptions in hot paths.
- One class per header/source pair; filenames match the class name (`PascalCase`).
- Namespaces mirror the folder: `core::`, `compositor::`, `shell::`, `apps::`.
- No comments unless the *why* is non-obvious.
- Default to no new files — prefer editing existing ones.

## Build

```bash
cmake -S . -B build
cmake --build build --parallel
./build/csopesy
```

Requires: CMake ≥ 3.16, GCC 11+ / Clang 13+ / MSVC 2022 (C++20), OpenGL 3.3.
Linux also needs: libX11/libXrandr/libXinerama/libXcursor/libXi dev headers.
FetchContent downloads GLFW and Dear ImGui on first configure (needs network).

## Shutdown rule

The app **must** be closed via the PWR button. Do not force-quit during demos.
