# microwave design review

Revision: rev1
Reviewer: <name>
Date: <yyyy-mm-dd>
Outputs reviewed: `review/microwave-rev1-*` from `kicad/tools/release.sh`

Mark each item `[x]` when it passes, or leave it open with a note under Findings. A revision is
released only when every item passes or has a recorded exception.

## Schematic

- [ ] ERC reports 0 errors and 0 warnings, or each exclusion has a written reason
- [ ] Every IC pin matches its datasheet pinout and function
- [ ] MCU pin assignments match the firmware pin map in `fw/`
- [ ] Every IC supply pin has its datasheet decoupling, placed on that pin's net
- [ ] Voltage ratings are at least 2x the rail, and power and current ratings cover worst case
- [ ] No floating inputs, no single-pin nets, signal nets named
- [ ] Every symbol has Footprint, MPN, Manufacturer, and a supplier part number
- [ ] DNP and Exclude from BOM flags match what gets bought and placed
- [ ] Title block carries board name and revision

## Layout

- [ ] DRC reports 0 errors against the PCB standard in `kicad/README.md`, or each exclusion has a written reason
- [ ] Schematic parity: 0 unconnected pads, 0 footprint errors
- [ ] Every footprint matches its datasheet land pattern, pin 1, and polarity
- [ ] Pin 1, polarity, and connector pinout are marked on silkscreen
- [ ] Track widths follow the net classes (10 mil signal, 20 mil power, 60 mil input)
- [ ] Every GND pour connects to the main plane, with no isolated pockets
- [ ] Switching loops (input cap, switch, diode, inductor) are short and compact
- [ ] Board outline, mounting holes, and fixed parts match the mechanical drawing in `mech/`
- [ ] Courtyard overlaps and other DRC exclusions are reviewed and justified
- [ ] Silkscreen shows refdes, board name, and revision, and is readable
- [ ] 3D view shows no part collisions, and tall parts clear the enclosure

## Fabrication package

- [ ] `fab/microwave-rev1-gerbers.zip` opens in a Gerber viewer and shows every copper, mask, silk, and outline layer
- [ ] PTH and NPTH drill files are present and line up with the copper
- [ ] Layer count and board size in the JLCPCB quote match the design

## Findings

| Item | Finding | Resolution |
| --- | --- | --- |
