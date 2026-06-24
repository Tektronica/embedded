# fw-nano-playKitchen — Project Context

Arduino Nano firmware for a toy play kitchen. First appliance: a **cooktop** — four WS2812 35-LED rings
as dimmable hob burners, one dimmer (potentiometer) per hob.

## Hardware constraints

- MCU: Arduino Nano (ATmega328) — **2 KB SRAM, 32 KB flash**. The LED framebuffer alone is
  140 × 3 = 420 B (~20% of SRAM). Budget memory deliberately.
- 5 V logic, drives WS2812 data directly (no level shifter).
- Power, wiring, and pinout are authoritative in `docs/hardware/`.

## Code conventions

- **No dynamic allocation** in the hot path — avoid `new`, `String`, `std::vector`,
  `std::function`, RTTI. Static allocation only.
- **MVC + Strategy:** Model (pure state/logic, no hardware) · View (`LedRenderer`, owns the
  FastLED framebuffer) · Controller (`InputController`, reads dimmers). Appliances implement the
  `IAppliance` plugin interface, selected at **compile time** per environment.
- **Central pin config** in `include/` — no module hardcodes a pin; pins are passed in at init
  (dependency injection). No layer "owns" the pins.
- Library: **FastLED** (HSV + `setMaxPowerInVoltsAndMilliamps` current limiting).
- Keep patterns light — principles over the pattern zoo (see `docs/architecture/`).

## Naming

- Repo name = `[codingType]-[targetPlatform]-[projectType]` → `fw` · `nano` · `playKitchen`.
  Hyphens delimit slots; multi-word slot values are camelCase (`playKitchen`).
- Repo boundary follows the code-sharing boundary: a different board is a *separate* repo
  (`fw-esp32-playKitchen`) unless it shares this codebase, in which case it's colocated here as
  another PlatformIO environment.
- Disk grouping/category buckets use a snake `_` prefix (`_embedded`).

## Build / flash

```bash
pio run                 # build
pio run -t upload       # build + flash
pio device monitor      # serial monitor
```

Toolchain is VSCode + PlatformIO (no Arduino IDE). Details in `docs/setup/`.

## Committing

- Use the **`/commit`** command (`.claude/commands/commit.md`) — it encodes the format, batching,
  and pre-commit checks.
- **Local commits only**, never push. **No `Co-Authored-By`** / AI attribution in messages.
- Personal repo — no Fluke (or any company) copyright headers on source.
- Pre-commit gate: `pio run` (0 errors/warnings) + `pio check` (no high-severity defects).

## Docs

`docs/` holds the detail: `setup/`, `hardware/`, `architecture/`, `appliances/`. Update the
relevant pillar when behavior changes.
