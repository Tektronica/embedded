---
description: Generate and create local commits from tracked changes (firmware project rules)
---

# Commit

Generate commit messages from tracked changes and create the commits locally, following the rules below.

---

## Commit Format

```
<type>(<scope>): <short imperative summary>

- What: <what changed — specific files, symbols, or behavior>
- Why: <the motivation — bug, gap, new requirement>
- How: <the mechanism — approach or pattern used>
- Benefit: <the outcome — what this enables or improves>
```

Keep each bullet to one sentence. Omit a bullet only if it adds no information beyond the summary line.

Scope is usually the appliance or subsystem (`cooktop`, `led`, `input`, `core`, `build`).

---

## Batching Rules

Unless the prompt specifies otherwise, group changes into logical commits rather than one large commit.

**Default groupings (commit in this order):**

1. **feat / fix / refactor / perf** — firmware source changes (`src/`, `lib/`, `include/`)
2. **build / chore** — `platformio.ini`, board/env config, tooling
3. **docs** — documentation updates (markdown, `docs/`)
4. **test** — test additions or updates (always last)

If a change spans multiple groups, split it. If a group has no changes, skip it.

A commit should be logically self-contained — reviewable in isolation without depending on an uncommitted sibling.

---

## Scope

Only commit locally. Never push to the remote. Do not add a `Co-Authored-By` trailer or any AI attribution to commit messages.

---

## Secrets File Guard

Before staging any file, check whether it matches a secrets pattern: `.env.*`, `secrets.h`,
`*_secrets.h`, `arduino_secrets.h`, `credentials.h`.

If any such file is staged or about to be staged, **stop and ask the user** before proceeding:

> "The following file(s) may contain secrets: `<file list>`. Commit them anyway?"

Only continue if the user explicitly confirms. If they decline, remove those files from the staging area and proceed without them.

---

## Pre-Commit Verification

Before committing, both checks must pass for the affected environment(s):

```bash
pio run -e <env>      # must compile with 0 errors and 0 warnings
pio check -e <env>    # static analysis — no high-severity defects
```

If you have off-device unit tests, also run `pio test -e native`.

If any check produces errors/warnings/defects, do not proceed. Report the failure reason and ask the user how to continue:

- **Fix now** — resolve the issues and re-run verification before committing
- **Defer** — commit anyway with a note that issues exist (only if user explicitly approves)
- **Abort** — cancel the commit entirely

---

## Type Reference

| type       | use for                                       |
| ---------- | --------------------------------------------- |
| `feat`     | new capability or behavior                    |
| `fix`      | corrects broken or incorrect behavior         |
| `refactor` | restructures without changing behavior        |
| `perf`     | memory, timing, or power improvements         |
| `docs`     | documentation only                            |
| `test`     | tests only                                    |
| `build`    | `platformio.ini`, board/env, dependencies     |
| `chore`    | tooling, scripts, misc maintenance            |

---

## Example

```
feat(cooktop): map dimmer position to hob heat ramp

- What: added the heat-ramp mapping in lib/cooktop and wired InputController to feed it
- Why: each hob needs to render a heat level derived from its dimmer reading
- How: linear ADC→level, then an HSV ramp (deep red → orange → yellow-white) via FastLED
- Benefit: turning a dimmer now changes that burner's color and brightness as expected
```
