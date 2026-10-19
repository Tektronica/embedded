# hw

Shared hardware layer for every board in this repo. Each board's own design package, BOM included,
lives in `projects/<name>/hw/`. This folder holds only what every board shares:

| Folder | Contents |
| --- | --- |
| `kicad/` | `embedded.kicad_sym`, the shared KiCad symbol library |
| `parts/` | `parts.csv`, the part catalog, and `PARTS_POLICY.md`, the rules that pick each part |
| `templates/` | `REVIEW.md`, the design review checklist each board starts from |
| `tools/` | `release.sh`, `combine_boms.py`, `print_review.py` |

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
| Library Path | `${KIPRJMOD}/<up>/hw/kicad/embedded.kicad_sym` |

`${KIPRJMOD}` is the folder holding the `.kicad_pro`. `<up>` is one `..` per folder between it and
the repo root. For `projects/<name>/hw/kicad/`, that is `../../../..`.

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

## Parts

`parts/parts.csv` records each selected part once, with its rule, MPN, supplier part numbers, status, and
the search notes behind the pick. It is a catalog, not a BOM. `parts/PARTS_POLICY.md` is the rule set that picks each one: hard
filters, a ranked manufacturer family per category, and a fixed tie-break, so the same inputs always
give the same part.

The pick is written into the schematic as symbol fields: `MPN`, `Manufacturer`, `Digi-Key`,
`Mouser`, and `LCSC` where one exists. Quantities and reference designators come from the
schematic only, so each board's BOM exports with identical columns and no second source to drift.

`tools/combine_boms.py` merges every board's `hw/bom/*-bom.csv` into one order list grouped by
MPN, with a per-board breakdown. Run it when placing an order that covers more than one board. It
prints to stdout, or to a file given as its argument, and the result isn't committed.

## Board design package

Every board uses the same layout:

```
projects/<name>/
  fw/                    PlatformIO firmware
  hw/
    kicad/               <name>.kicad_pro, .kicad_sch, .kicad_pcb, sym-lib-table
    datasheets/          PDFs for the parts on this board
    mech/                enclosure drawings, exported STEP
    bom/<name>-bom.csv   Reference, Qty, Value, Footprint, MPN, Manufacturer, Digi-Key, Mouser, LCSC
    fab/                 JLCPCB upload: gerbers/ (Gerbers, PTH and NPTH drill, IPC-D-356 netlist) and <name>-<rev>-gerbers.zip
    review/              schematic and layout PDFs, KiCad netlist, 3D renders, ERC and DRC reports,
                         REVIEW.md and its printable <name>-<rev>-review-checklist.pdf
```

`tools/release.sh projects/<name>/hw` regenerates `bom/`, `fab/`, `review/`, and the STEP file from the
KiCad project. The revision comes from the schematic title block (`rev` field) and is stamped into
every output name. `--assembly` also writes a JLCPCB CPL and assembly BOM into `fab/`, for when
JLCPCB places parts.

Gerbers follow JLCPCB's KiCad guide: copper, mask, silkscreen, and Edge.Cuts layers, Protel
extensions, X2 with netlist attributes, and Excellon drill in mm with separate PTH and NPTH files,
zipped into one archive for the uploader. The IPC-D-356 netlist rides in the same zip for the fab
house's bare-board electrical test.

A revision is released when `review/REVIEW.md` is complete. Each board's checklist starts from
`templates/REVIEW.md` and is filled in with that board's parts, pins, and expected DRC results.
`release.sh` renders it to `review/<name>-<rev>-review-checklist.pdf` (through Chrome) for printing.
Commit the outputs together with the signed-off checklist, then tag the commit `<name>-<rev>`.

## PCB standard

Target fab is JLCPCB, 2 layers. Every board uses these sizes, in mil, so a track's width tells
you what it carries.

| Item | Size | Use |
|---|---|---|
| Signal track | 10 mil | Logic, PWM, analog inputs |
| Power track | 20 mil | Regulated rails, LED and switch nodes, GND stubs |
| Input track | 60 mil | Unregulated input power |
| Clearance | 8 mil | Default net class |
| Via | 24 mil pad, 12 mil drill | Every via |
| Zone | 20 mil clearance, 10 mil minimum width | Every pour |

Each project carries a `Default` net class (signal) and a `Power` net class matched by net name
pattern, so new tracks come out at the right width. Place on a 25 mil grid. Check these against
JLCPCB's current capabilities page before ordering.
