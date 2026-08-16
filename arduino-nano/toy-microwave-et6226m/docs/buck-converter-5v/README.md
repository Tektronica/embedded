# 5V Rail: Two Sources, One Rail

This board has two ways to get 5V. Both can be connected at the same time. This doc explains why
that's safe.

## The two sources

- **12V → buck converter → 5V.** Normal operation. Powers the Nano, the ET6226M display, the
  buzzer.
- **USB → the Nano's own onboard regulator → 5V.** Used for programming/debugging. Comes in
  through the Nano module itself, not a separate chip on this board.

Both paths land on the same physical net: `+5V`.

## Why that's a problem by default

Two live voltage sources on one rail means whichever is higher pushes current into the other.
Worst case: the buck converter backfeeds through the Nano's 5V pin, out through its USB port, and
into your computer. Best case: nothing breaks, but it's not designed, it's luck.

## The fix: one diode per source

**D17** (SS14 Schottky) sits between the buck converter's raw output and the shared `+5V` net.
Current flows converter → rail. Never rail → converter.

**The Nano module already has its own diode** (SS1P3L on Gravitech's board, between VUSB and its
internal 5V rail). Current flows USB → rail. Never rail → USB.

Result: each source is independently protected. Whichever source has the higher voltage at any
moment supplies the current; the other diode reverse-biases and does nothing. Doesn't matter which
one wins — neither can backfeed the other.

**Don't add a diode on the Nano's 5V pin.** That pin has to work two ways: output when
USB-powered, input when the buck is powering it with no USB connected. A diode can only allow one
direction. Putting one there breaks 12V-only operation. Populate that connection as a plain trace
or 0Ω jumper — never a diode.

## NetTie, not a plain wire

The Nano's 5V pin and D17's cathode both look like "power output" pins to KiCad's ERC. Wiring them
directly into one `+5V` net trips a "multiple power-output pins on the same net" warning.

Fix: name them as two distinct nets (`+5V_NANO`, `+5V`) and bridge them with a **NetTie-2**
symbol. Same electrical connection, no false-positive ERC warning. Same technique used for
star-ground splits (AGND/DGND tied at one deliberate point) — here it's two power sources instead.

## Buck converter: TPS563200 → AP63205 (done)

| | TPS563200 (old) | AP63205 (current, U1) |
|---|---|---|
| Output | Adjustable (needed a feedback divider) | **Fixed 5V** — FB pin traces straight to the buck's raw output |
| Feedback parts | R9 (56.2k), R10 (10k) | **Deleted** |
| Max current | 3A | 2A — still far above this board's <500mA load |
| Efficiency @ 12V→5V | — | ~90-93%, confirmed in datasheet test curves |
| Package | SOT-23-6 | TSOT-23-6 |
| Price | — | <$1 |

Why switched: fixed-output meant deleting two resistors and one thing that could be mis-valued.
Confirmed 2A is real (tested at this exact 12V-in/5V-out condition in the datasheet), not a
best-case number. Confirmed in active production, not pre-release.

## Component values (as built)

| Component | Old (TPS563200) | Now (AP63205) |
|---|---|---|
| L1 (inductor) | 3.3µH | **4.7µH** |
| C6, C7 (output caps) | 10µF each | **22µF each** |
| C5 (bootstrap cap) | 100nF | 100nF — no change |
| R9, R10 (feedback divider) | 56.2k / 10k | **Deleted** |

## Inductor: Bourns SRN8040-4R7Y vs. Murata LQH5BPZ4R7NT0L

Both meet spec (4.7µH, Isat ≥2.7A, DCR <100mΩ, genuinely hand-solderable — wraparound
terminations, not bottom-pad-only LGA). Don't assume "datasheet says reflow" means not
hand-solderable — nearly every SMD part's datasheet says that; the real test is whether the
termination is visible/accessible from the side, not what the manufacturer lists as their
production-line method.

**Picked: Murata LQH5BPZ4R7NT0L** — smaller footprint (5.0×5.0×2.2mm vs. Bourns' 8.0×8.0×4.0mm),
more stock depth (2,297 vs. 187 units), cheaper ($0.46 vs. $0.55). Isat (3A) and DCR (69.6mΩ) are
worse than the Bourns part on paper, but this board's actual draw (~0.5A) is nowhere near either
part's limit, so the difference doesn't matter in practice.

**Not yet done: L1's footprint in the schematic is still `L_Murata_LQH55DN_5.7x5.0mm`** — a
different Murata series than the one actually picked. Needs updating to match LQH5BPZ4R7NT0L's
real footprint before layout is final.

## Still to verify

- **L1 footprint mismatch** (see above) — update to match LQH5BPZ4R7NT0L.
- AP63205 vs TPS563200 pin-for-pin compatibility (don't assume same package name = same pinout;
  not independently re-checked since the swap).
- Where the input capacitor actually is / whether it meets the >10µF ceramic recommendation.
