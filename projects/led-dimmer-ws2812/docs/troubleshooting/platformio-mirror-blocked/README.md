# PlatformIO package downloads blocked (Zscaler) — use GitHub source

**Status:** Resolved (worked around)

## Symptom

- `pio run` hangs for many minutes, then the log loops:
  `Tool Manager: Looking for another mirror...` while installing `tool-scons` (or any package).
- VSCode "Resolving dependencies…" never finishes; PIO Home times out.

## Cause

PlatformIO package files 302-redirect from `dl.registry.platformio.org` to the mirror
`*.contabostorage.com`, which **Zscaler blocks at the connection/category level** (not just DNS —
DoH resolves the IP, but the connection is still killed). The registry *API* works, so pio thinks
the package exists but can never download it. See also [zscaler-platformio-ssl](../zscaler-platformio-ssl/).

## Fix / workaround

- **Libraries:** point `lib_deps` at the **GitHub** source instead of the registry (GitHub is
  allowed by Zscaler). In `platformio.ini`:
  ```ini
  lib_deps =
      https://github.com/FastLED/FastLED/archive/refs/tags/3.7.0.tar.gz
  ```
- **Core packages** (`tool-scons`, toolchains): these only live on the blocked mirror. Options:
  - have IT allow-list `*.contabostorage.com` + `dl.registry.platformio.org`, or
  - run the one-time download on a non-Zscaler Apple-Silicon Mac and copy `~/.platformio/{packages,platforms}` over.
  - (The `native` test platform + Unity downloaded fine — only some mirrors are blocked.)

Once packages are cached in `~/.platformio`, builds/tests/uploads work offline on the corporate network.

## Avoid the deadlock

Don't run a CLI `pio run` while VSCode is also "Resolving dependencies" — they fight over
`~/.platformio/packages.lock` and stall. Let one finish, or `pkill -f "platformio -c vscode"` and
clear stale `*.lock` files before retrying.
