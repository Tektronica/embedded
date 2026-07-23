# embedded — Monorepo Context

A personal monorepo of Arduino/embedded firmware projects, one PlatformIO project per board.

## Folder / naming scheme

- `[board]/[kebab-case-project]/` — board = chip/platform (`arduino-nano`, `attiny`, `esp32`, ...),
  project in kebab-case (e.g. `arduino-nano/led-dimmer-ws2812/`).
- Each project folder is fully self-contained: its own `platformio.ini`, `include/`/`src/`/`test/`,
  `docs/`, `README.md`, and `CLAUDE.md`.
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
