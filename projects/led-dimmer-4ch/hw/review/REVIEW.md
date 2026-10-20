# LED Dimmer rev 1 design review

Board: led-dimmer-4ch (4-channel LED dimmer)
Revision: rev 1
Reviewer: ____________________
Date: ____________________

Tick each box when the item passes. If something fails, write it in the Findings table at the end
and leave the box empty. The board is released when every box is ticked or has a finding with a
resolution.

## 1. Before you start

- [ ] Regenerate the outputs: `tools/release.sh projects/led-dimmer-4ch/hw`
- [ ] Have these open next to KiCad: the schematic PDF, the layout PDF, and the two 3D images in this folder

## 2. Schematic

- [ ] ERC report shows 0 errors and 0 warnings
- [ ] PT4115 pins (U1-U4) match their datasheet: SW, GND, DIM, CSN, VIN
- [ ] AP63205 pins (U5) match its datasheet: FB, EN (tied to VIN), VIN, GND, SW, BST
- [ ] Nano pins match the firmware: PWM on D9, D10, D3, D11 for channels 1-4, pots on A0-A3
- [ ] Each channel's sense resistor is 0.36 Ω, setting 278 mA LED current (0.1 V ÷ 0.36 Ω)
- [ ] Each PT4115 DIM line has a 10 kΩ pull-down (R5-R8)
- [ ] Each PT4115 has a 4.7 µF input cap, a 100 µH inductor, and an SS14 catch diode
- [ ] 5 V buck (U5): 10 µF input cap, 22 µF output caps (×2), 4.7 µH inductor, 100 nF boot cap
- [ ] Caps on the 12 V rail are rated at least 25 V, and the SS14 diodes are 40 V
- [ ] Pot inputs have 100 nF filter caps (C9-C12)
- [ ] Every part in the BOM has a manufacturer part number and a supplier part number
- [ ] The Nano is left out of the BOM on purpose
- [ ] Title block shows rev 1

## 3. Layout

- [ ] DRC report shows 0 violations, 0 unconnected pads, and 0 footprint errors
- [ ] Footprints match their datasheets: PT4115 (SOT-89-5), AP63205 (TSOT-23-6), MSS1038 inductor, 0.36 Ω sense resistor
- [ ] Pin 1 and polarity are marked on silkscreen for every diode, IC, and terminal block
- [ ] Track widths are only 10, 20, or 60 mil, and every via is 24/12 mil
- [ ] Ground pours have no isolated islands
- [ ] Each PT4115 loop is compact: input cap, PT4115, inductor, and catch diode sit close together
- [ ] Buck converter loop is compact: input cap, AP63205, inductor, and switch-node pour sit close together
- [ ] All four channel blocks are laid out the same way
- [ ] Board outline is 3.1 × 2.6 in with a mounting hole in each corner
- [ ] The Nano's USB end sits at the board edge
- [ ] Terminal blocks are Phoenix MKDS 5.08 mm, and pin 1 is marked on each
- [ ] LED wire order on J2 and J3, top to bottom: LED+, LED-, LED+, LED- for channels 1-2 and 3-4
- [ ] Silkscreen shows reference designators, the board name, and rev 1, and none of it is hidden under a part
- [ ] 3D view shows no parts colliding

## 4. Fab files

- [ ] The gerber zip opens in a Gerber viewer and shows front and back copper, mask, silkscreen, and the board outline
- [ ] The zip also holds the plated and non-plated drill files and the IPC-D-356 netlist
- [ ] Drill holes line up with their pads
- [ ] JLCPCB quote page detects 2 layers and 3.1 × 2.6 in

## 5. Sign-off

- [ ] Reviewer name and date filled in above
- [ ] Findings table complete, every finding resolved
- [ ] This file committed, and the commit tagged `led-dimmer-4ch-rev1`

## Findings

| # | Item | Finding | Resolution |
| --- | --- | --- | --- |
| 1 | | | |
| 2 | | | |
| 3 | | | |
| 4 | | | |
| 5 | | | |
