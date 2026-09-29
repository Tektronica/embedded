# kicad

Shared KiCad symbol library for every project in this repo: `embedded.kicad_sym`.

Symbols edited inside KiCad's bundled libraries (under the app install) are overwritten by the
next KiCad update. Custom or corrected symbols live here instead, versioned with the repo.

A schematic keeps its own cached copy of every symbol it uses, so an existing board survives a
lost library. The library is still the source for "Update Symbols from Library" and for any new
project.

## Register the library in a project

Preferences > Manage Symbol Libraries > Project Specific Libraries, add a row:

| Field | Value |
|---|---|
| Nickname | `embedded` |
| Library Path | `${KIPRJMOD}/<up>/kicad/embedded.kicad_sym` |

`${KIPRJMOD}` is the folder holding the `.kicad_pro`. `<up>` is one `..` per folder between it and
the repo root. For `<board>/<project>/KiCad/<name>/`, that is `../../../..`.

This writes a `sym-lib-table` next to the `.kicad_pro`. Commit it.

## Add a symbol

1. Symbol Editor: find the source symbol (a KiCad library or another project), right-click > Copy.
2. Right-click the `embedded` library > Paste. Or File > New Symbol to start from scratch.
3. Edit, then save the library.

## Move a placed symbol onto this library

1. Add the symbol to `embedded` (above).
2. In the schematic, right-click the symbol > Change Symbol, pick `embedded:<name>`, keep existing
   fields. Tools > Change Symbols does every instance at once.
3. Run ERC. Zero library warnings means the link resolves.
