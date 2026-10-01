# Part selection policy

How a manufacturer part number (MPN) is chosen for any part on any board in this repo. The
chosen parts live in `parts.csv`. Every row names the rule below that produced it, so re-running
the same rules gives the same answer.

A pick is made in three passes, in order. A part moves to the next pass only if it survives the
one before.

1. **Hard filters** (global, below). Anything failing one is out, whatever the price.
2. **Category rule.** The category's spec and its ranked manufacturer families. Take the first
   family that has a part meeting the spec. An unordered tier sends every family's qualifying
   parts to the tie-break together.
3. **Tie-break** (global, below). Only used when more than one qualifying part is left.

## Hard filters

| Filter | Rule |
| --- | --- |
| Footprint | The part's datasheet land pattern and pin 1 match the symbol's footprint. The footprint is chosen first; the part fits it, never the reverse. |
| Lifecycle | Active. Not NRND, last-time-buy, or obsolete. |
| Stock | In stock at Digi-Key **or** Mouser for at least 10x the planned quantity. Both is preferred (tie-break 1). |
| Package | Hand-solderable: 0805 or larger passives, no leadless packages without exposed leads, solderable terminations only (no epoxy-mount / conductive-adhesive terminations; flexible terminations are fine). |
| Compliance | RoHS. |
| Seller | Sold and shipped by Digi-Key or Mouser itself, not a marketplace or third-party listing. |

## Tie-break (in order)

1. Stocked at both Digi-Key and Mouser.
2. No tariff flagged by the distributor for the shipping destination. Preferred, not required.
3. Lower unit price at the planned quantity.
4. Tighter tolerance or higher rating, at the same price.
5. Lowest MPN alphabetically. This exists only so the same inputs always give the same pick.

Search one row at a time with the distributor's parametric filters set to the row's spec, not narrower: select every tolerance the spec allows (±20% or tighter means ±20%, ±10%, and ±5%), every voltage at or above the minimum, and the footprint's case size.

## Category rules

| Rule | Category | Spec | Families, in rank order | MPN derivation |
| --- | --- | --- | --- | --- |
| `R-0805` | Resistor, general: pull-up/down, gate, series, LED limiting | 0805, thick film, 1%, at least 1/8 W | Yageo RC, Panasonic ERJ-6ENF, Vishay CRCW0805 | Yageo: `RC0805FR-07<code>L`, code as `100R`, `2K`, `4K7`, `10K`. `-07` is the 7" reel sold as cut tape; `-10`/`-13` are the same part on larger reels, and the `…P` suffix is the pricier RC_P series. |
| `R-PRECISION` | Resistor, precision analog: dividers into an ADC or reference, feedback dividers, op-amp gain, audio | 0805, thin film, 0.1%, at most 25 ppm/°C | Panasonic ERA-6A, Susumu RG, Vishay TNPW | Family part-number table |
| `R-SENSE` | Resistor, current sense or current set (shunts, LED-driver Rs) | 0805 or larger, metal film, metal element, or metal strip, 1%, at most 100 ppm/°C, at least 1/4 W | Vishay WSL, Yageo PE/PT, Panasonic ERJ-6BW, Susumu PRL, Ohmite KDV | Family part-number table |
| `R-PULSE` | Resistor, inrush, snubber, ESD or pulse paths | Anti-surge thick film, rated for the pulse energy | Panasonic ERJ-P06, Vishay CRCW-HP, Yageo SR | Family part-number table |
| `C-C0G` | Capacitor, 1 nF and below | 0805, C0G/NP0, 50 V, ±5% or tighter | Murata GRM, TDK C, KEMET C, Taiyo Yuden, Samsung CL (tier 1, unordered) | Family part-number table |
| `C-X7R` | Capacitor, above 1 nF, below 10 µF | 0805 or 1210 per footprint, X7R, ±20% or tighter, at least 2x the rail and never below 25 V on the 12 V rail | Murata GRM, TDK C, KEMET C, Taiyo Yuden, Samsung CL (tier 1, unordered) | Family part-number table |
| `C-BULK` | Capacitor, 10 µF and up | 1210, X7R preferred, X5R allowed, ±20% or tighter, at least 2x the rail and never below 25 V on the 12 V rail | Murata GRM, TDK C, KEMET C, Taiyo Yuden, Samsung CL (tier 1, unordered) | Family part-number table |
| `D-SCHOTTKY` | Schottky rectifier | SMA, 1 A, at least 40 V | Vishay, onsemi, Taiwan Semiconductor, Diodes Inc | Base part `SS14` plus the maker's packaging suffix |
| `D-SIGNAL` | Small-signal diode | SOD-123 (not SOD-123F or SOD-323), 1N4148W | Diodes Inc, onsemi, Vishay | Base part `1N4148W` plus the maker's packaging suffix |
| `L-POWER` | Power inductor | The footprint's exact series; Isat at least 1.5x peak current; lowest DCR in that series | The footprint's maker only | Series plus value code |
| `TERMINAL` | PCB screw terminal | 5.08 mm (200 mil) pitch, fixed, horizontal entry; the footprint's exact series | The footprint's maker only | The maker's order number for the position count |
| `EXACT` | Specific IC, transistor, display | The symbol's exact part | The one maker | The datasheet's orderable part number, including tape/reel suffix |
| `LCSC` | Part not carried by Digi-Key or Mouser | As `EXACT` | The one maker | Ordered from LCSC; record the LCSC part number |
| `OWNED` | Part already in stock at home | Matches the footprint | Whatever you have | Record what you have; buy only to top up |
| `OFFBOARD` | Module, switch, or anything not soldered from this BOM | n/a | n/a | Marked Exclude from BOM in KiCad; listed here for completeness |

Resistors are categorized by function, not value: the function picks the technology, and the
technology picks the families. Thick film drifts more (±100-200 ppm/°C) and has more excess noise
than thin film, which matters in high-impedance precision paths; sense parts are metal film or
metal element, which hold ±50-100 ppm/°C down to milliohms. Capacitors follow the same idea: C0G
for filters and timing, X7R for decoupling, never Y5V or Z5U. Tolerance only matters where the
capacitor sets a time constant or filter corner, which makes it a C0G part; for X7R/X5R
decoupling, DC-bias loss (30-50% on a small bulk part) swamps ±10% vs ±20%, so tie-break 4 takes
±10% only when it costs the same.

A family list is justified only by a stated, checkable criterion. The capacitor tier is the
makers that publish per-part DC-bias curves (Murata SimSurfing, TDK SEAT, KEMET K-SIM, Taiyo
Yuden, Samsung), since the category specs depend on capacitance under bias.

The tier has no internal order, since any order would be arbitrary, so the tie-break picks the
maker. Mixing makers across rows costs nothing on a hand-built board.

Every resistor, diode, and sense family order above is arbitrary: it only fixes which maker to try
next. Changing a list is a policy change: update this table, then re-derive every row that uses the
rule.

When a pick fails a hard filter on a distributor page, keep the row, move to the next family, and
record the rejected MPN, date, and reason in Notes (for example `Rejected GRM...: out of stock,
NRND (2026-09-30)`). A rejected MPN is never re-picked until the note is removed.

## Status values in `parts.csv`

| Status | Meaning |
| --- | --- |
| `open` | Rule chosen, MPN not picked yet |
| `candidate` | Derived from the rules but not yet checked on a distributor page |
| `verified` | Checked on a distributor page: passes every hard filter. Record the date and what was checked in Notes. Supplier part numbers are optional, since the Digi-Key and Mouser BOM upload tools match on MPN. |
| `owned` | Using stock on hand |
| `lcsc` | Only available from LCSC |
| `excluded` | Not bought from this BOM |
| `blocked` | Can't be picked until a design decision is made (see Notes) |

Only `verified`, `owned`, and `lcsc` rows are ready to order.
