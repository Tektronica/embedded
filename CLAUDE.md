# embedded: monorepo context

A personal monorepo of embedded projects: firmware, and for some projects a custom PCB.

## Layout

| Folder | Holds | Grouped by |
| --- | --- | --- |
| `projects/<name>/` | Things you build. `fw/` holds firmware, `hw/` holds the PCB design package when the project has a custom board, `docs/` holds design notes. | Product |
| `examples/<platform>/<name>/` | Single-concept demos, each a self-contained firmware project | Platform (`arduino-nano`, later `raspberry-pi`) |
| `lib/<platform>/` | Shared libraries: `arduino/<Name>/` for firmware, `kicad/` for the symbol library | Platform (`arduino`, `kicad`, later `python`) |
| `tools/` | Hardware release tools: `release.sh`, `combine_boms.py`, `print_review.py` | (shared) |
| `orders/` | One folder per Digi-Key or Mouser order, named `<yyyy-mm-dd>-<supplier>` | (shared) |
| `docs/` | Cross-project notes. Each topic is a folder ending in `README.md` (`docs/hw/part-selection/README.md`). | (shared) |

- Names are kebab-case, except libraries, which are PascalCase.
- A project on a second platform gets a folder named for its role next to `fw/`. A Raspberry Pi
  app goes in `projects/<name>/pi/`.
- A project's `hw/` follows the board design package in `docs/hw/README.md`. Each project owns its
  own BOM in `hw/bom/`. `docs/hw/part-selection/parts.csv` is the part catalog, not a BOM.
- Every project and example has a top-level `README.md` stating what it is, the firmware platform,
  the tooling, and how to build, flash, and test it.

## Libraries

- Logic lives once in `lib/<platform>/<Name>/` when two or more projects use it, or when a project
  exists to demonstrate it. Arduino libraries are header-only and hardware-free where possible,
  each with its own `library.json`, `platformio.ini`, and `test/`.
- Projects pull a library in with `lib_deps = symlink://../../../lib/arduino/<Name>` in every env
  that includes it. A project folder no longer builds when copied out of the repo on its own.
- Libraries don't depend on each other. A library includes only `stdint.h`, or the Arduino core
  for a hardware driver. The app composes libraries by passing plain values between them (a raw
  reading, a percent, a segment byte) and supplies every tunable (pins, thresholds, frequencies,
  channel counts). Two libraries may depend on each other only when they're colocated as one
  package (a driver and its codec) or the exception is written down in both READMEs.

## Toolchain

- VS Code with PlatformIO for every firmware project. No Arduino IDE.
- `pio` commands take `-d <project-dir>` when run from the repo root.
- KiCad 10 for hardware. `tools/release.sh projects/<name>/hw` regenerates a board's outputs.

## Committing

- Use the `/commit` skill (`.claude/commands/commit.md`). It works across every project.
- Local commits only, never push. No `Co-Authored-By` or other AI attribution. Personal repo, so no
  company copyright headers on source.

## Docs

Each project keeps its own `README.md` and `docs/` tree, and a `CLAUDE.md` only for conventions
that apply to that project alone. This file covers what's shared.
