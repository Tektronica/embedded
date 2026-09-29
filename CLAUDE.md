# embedded — Monorepo Context

A personal monorepo of Arduino/embedded firmware projects, one PlatformIO project per board.

## Folder / naming scheme

- `[board]/[kebab-case-project]/` — board = chip/platform (`arduino-nano`, `attiny`, `esp32`, ...),
  project in kebab-case (e.g. `arduino-nano/led-dimmer-ws2812/`).
- Each project folder is self-contained except for shared libraries: its own `platformio.ini`,
  `include/`/`src/`/`test/`, `docs/`, `README.md`, and `CLAUDE.md`.
- Logic lives once in `lib/<Name>/` (header-only, hardware-free, its own `library.json` and
  `test/`) when two or more projects use it, or when a project exists to demonstrate it. Projects
  pull it in with `lib_deps = symlink://<path>/lib/<Name>` in every env that includes it. A
  project folder no longer builds when copied out of the repo on its own.
- Libraries don't depend on each other. A library includes only `stdint.h`, or the Arduino core
  for a hardware driver. The app composes libraries by passing plain values between them (a raw
  reading, a percent, a segment byte) and supplies every tunable (pins, thresholds, frequencies,
  channel counts). Two libraries may depend on each other only when they're colocated as one
  package (a driver and its codec) or the exception is written down in both READMEs.
- A new board or a new project on an existing board is a new top-level folder here, not a new repo.

## Toolchain

- VSCode + PlatformIO for every project — no Arduino IDE.
- `pio` commands assume the working directory is the project folder; use `-d <project-dir>` when
  running from elsewhere (e.g. the repo root).

## Committing

- Use the **`/commit`** skill (`.claude/commands/commit.md`) — monorepo-aware, works across all
  projects.
- **Local commits only**, never push. **No `Co-Authored-By`** / AI attribution. Personal repo — no
  company copyright headers on source.

## Docs

Each project keeps its own `README.md`, `CLAUDE.md`, and `docs/` tree. This file covers only what's
shared across projects — see `<board>/<project>/CLAUDE.md` for project-specific conventions.
