# Microwave rev 1 design review

Board: microwave (toy microwave control panel)
Revision: rev 1
Reviewer: ____________________
Date: ____________________

Tick each box when the item passes. If something fails, write it in the Findings table at the end
and leave the box empty. The board is released when every box is ticked or has a finding with a
resolution.

## 1. Before you start

- [ ] Regenerate the outputs: `kicad/tools/release.sh arduino-nano/toy-microwave-et6226m/hw`
- [ ] Have these open next to KiCad: the schematic PDF, the layout PDF, and the two 3D images in this folder

## 2. Schematic

- [ ] ERC report shows 0 errors and 0 warnings
- [ ] ET6226M pins match its datasheet: GR1 pin 1, CLK 2, DAT 3, GND 4, GR2-GR4 pins 5-7, SG1-SG2 pins 8-9, VCC 10, SG3-SG7 pins 11-15, DP/KP 16
- [ ] Display wiring is in order: SG1-SG7 drive segments A-G, DP/KP drives DP, GR1-GR4 drive digits 1-4
- [ ] Display pins match the Kingbright datasheet: A 11, B 7, C 4, D 2, E 1, F 10, G 5, DP 3, digits 12, 9, 8, 6
- [ ] Key matrix: rows on SEG_A-SEG_D, columns on DIG1-DIG4 through 2 kΩ (R1-R4), one diode per key with the cathode on the row
- [ ] ET6226M support parts match the datasheet: 4.7 kΩ pull-ups (R7, R8), 100 Ω series (R5, R6), 100 pF to ground (C1, C2), 1 µF on VCC (C3)
- [ ] Nano pins match the firmware: CLK D8, DAT D9, buzzer D6, light D10, motor D11, fan D12, D13 not connected
- [ ] J2 reads pin 1 LIGHT, pin 2 MOTOR, pin 3 FAN, pin 4 GND, with a 10 kΩ pull-down on each signal (R11-R13)
- [ ] Buzzer driver: AO3400A (Q2) with 100 Ω gate resistor (R9), gate pull-down (R10), SS14 flyback diode (D18)
- [ ] 5 V buck (U1, AP63205): 10 µF input cap, 22 µF output caps, 4.7 µH inductor, all rated at least 25 V
- [ ] Every part in the BOM has a manufacturer part number and a supplier part number
- [ ] The Nano and the 16 key switches are left out of the BOM on purpose
- [ ] Title block shows rev 1

## 3. Layout

- [ ] DRC report shows 0 unconnected pads and 0 footprint errors
- [ ] DRC shows exactly 48 violations, and all of them are expected: 16 key-diode courtyard overlaps and 32 silkscreen-over-copper warnings
- [ ] Footprints match their datasheets: ET6226M (SOP-16), AP63205 (TSOT-23-6), AO3400A (SOT-23), LQH55DN inductor
- [ ] Pin 1 and polarity are marked on silkscreen for every diode, IC, and terminal block
- [ ] Track widths are only 10, 20, or 60 mil, and every via is 24/12 mil
- [ ] Ground pours have no isolated islands, including the pocket under the ET6226M
- [ ] Buck converter loop is compact: input cap, AP63205, inductor, and switch-node pour sit close together
- [ ] Board outline is 3.0 × 6.5 in with a mounting hole in each corner
- [ ] Display and key positions match the enclosure drawing (hw/mech/enclosure-template.dxf)
- [ ] Nothing on the front copper sits under the display or between the keys
- [ ] Terminal blocks are Phoenix MKDS 5.08 mm, and J2 pin 1 is marked
- [ ] Silkscreen shows reference designators, the board name, and rev 1, and none of it is hidden under a part
- [ ] 3D view shows no parts colliding

## 4. Fab files

- [ ] The gerber zip opens in a Gerber viewer and shows front and back copper, mask, silkscreen, and the board outline
- [ ] The zip also holds the plated and non-plated drill files and the IPC-D-356 netlist
- [ ] Drill holes line up with their pads
- [ ] JLCPCB quote page detects 2 layers and 3.0 × 6.5 in

## 5. Sign-off

- [ ] Reviewer name and date filled in above
- [ ] Findings table complete, every finding resolved
- [ ] This file committed, and the commit tagged `microwave-rev1`

## Findings

| # | Item | Finding | Resolution |
| --- | --- | --- | --- |
| 1 | | | |
| 2 | | | |
| 3 | | | |
| 4 | | | |
| 5 | | | |
