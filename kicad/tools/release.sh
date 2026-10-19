#!/usr/bin/env bash
# Regenerates a board's bom/, fab/, and review/ outputs from its KiCad project.
# Usage: kicad/tools/release.sh <project>/hw [--assembly]
set -euo pipefail

KICAD_CLI="${KICAD_CLI:-/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli}"
CHROME="${CHROME:-/Applications/Google Chrome.app/Contents/MacOS/Google Chrome}"

HW="${1:?usage: release.sh <project>/hw [--assembly]}"
ASSEMBLY="${2:-}"
HW="$(cd "$HW" && pwd)"
PRO="$(ls "$HW"/kicad/*.kicad_pro)"
NAME="$(basename "$PRO" .kicad_pro)"
SCH="$HW/kicad/$NAME.kicad_sch"
PCB="$HW/kicad/$NAME.kicad_pcb"

REV="$(sed -n 's/^[[:space:]]*(rev "\([^"]*\)").*/\1/p' "$SCH" | head -1)"
REV="rev${REV:-draft}"
BASE="$NAME-$REV"

rm -rf "$HW/fab"
mkdir -p "$HW/bom" "$HW/fab/gerbers" "$HW/review" "$HW/mech"
find "$HW/review" -type f ! -name REVIEW.md -delete

COPPER="F.Cu,B.Cu"
grep -q '"In1.Cu"' "$PCB" && COPPER="F.Cu,In1.Cu,In2.Cu,B.Cu"
FAB_LAYERS="$COPPER,F.Mask,B.Mask,F.Silkscreen,B.Silkscreen,Edge.Cuts"

echo "== $BASE"

"$KICAD_CLI" sch export bom "$SCH" -o "$HW/bom/$NAME-bom.csv" \
  --fields 'Reference,${QUANTITY},Value,Footprint,MPN,Manufacturer,Digi-Key,Mouser,LCSC' \
  --labels 'Reference,Qty,Value,Footprint,MPN,Manufacturer,Digi-Key,Mouser,LCSC' \
  --group-by 'MPN,Value,Footprint' --exclude-dnp >/dev/null

"$KICAD_CLI" pcb export gerbers "$PCB" -o "$HW/fab/gerbers/" -l "$FAB_LAYERS" --check-zones >/dev/null
"$KICAD_CLI" pcb export drill "$PCB" -o "$HW/fab/gerbers/" --format excellon --drill-origin absolute \
  --excellon-units mm --excellon-zeros-format decimal --excellon-separate-th >/dev/null
"$KICAD_CLI" pcb export ipcd356 "$PCB" -o "$HW/fab/gerbers/$BASE-ipc356.ipc" >/dev/null
(cd "$HW/fab/gerbers" && zip -q -j "../$BASE-gerbers.zip" ./*)

if [ "$ASSEMBLY" = "--assembly" ]; then
  "$KICAD_CLI" pcb export pos "$PCB" -o "$HW/fab/$BASE-cpl.csv" --format csv --units mm --exclude-dnp >/dev/null
  sed -i '' '1s/.*/Designator,Val,Package,Mid X,Mid Y,Rotation,Layer/' "$HW/fab/$BASE-cpl.csv"
  "$KICAD_CLI" sch export bom "$SCH" -o "$HW/fab/$BASE-bom-jlcpcb.csv" \
    --fields 'Value,Reference,Footprint,LCSC' --labels 'Comment,Designator,Footprint,LCSC Part #' \
    --group-by 'Value,Footprint,LCSC' --ref-range-delimiter '' --exclude-dnp >/dev/null
fi

"$KICAD_CLI" sch erc "$SCH" -o "$HW/review/$BASE-erc.rpt" >/dev/null
"$KICAD_CLI" pcb drc "$PCB" -o "$HW/review/$BASE-drc.rpt" --schematic-parity --refill-zones >/dev/null
"$KICAD_CLI" sch export netlist "$SCH" -o "$HW/review/$BASE.net" --format kicadsexpr >/dev/null
"$KICAD_CLI" sch export pdf "$SCH" -o "$HW/review/$BASE-schematic.pdf" >/dev/null
"$KICAD_CLI" pcb export pdf "$PCB" -o "$HW/review/$BASE-layout.pdf" --mode-multipage \
  -l "F.Fab,$COPPER,F.Mask,B.Mask,F.Silkscreen,B.Silkscreen,B.Fab" --cl Edge.Cuts \
  --ibt --sp --check-zones >/dev/null
"$KICAD_CLI" pcb render "$PCB" -o "$HW/review/$BASE-3d-top.png" --side top --quality high -w 1600 -h 1600 >/dev/null
"$KICAD_CLI" pcb render "$PCB" -o "$HW/review/$BASE-3d-bottom.png" --side bottom --quality high -w 1600 -h 1600 >/dev/null
"$KICAD_CLI" pcb export step "$PCB" -o "$HW/mech/$NAME.step" --subst-models >/dev/null 2>&1 \
  || echo "   step export skipped (missing 3D models)"

if [ -f "$HW/review/REVIEW.md" ]; then
  "$(dirname "$0")/print_review.py" "$HW/review/REVIEW.md" "$HW/review/.review.html"
  "$CHROME" --headless --disable-gpu --no-pdf-header-footer \
    --print-to-pdf="$HW/review/$BASE-review-checklist.pdf" "file://$HW/review/.review.html" >/dev/null 2>&1 \
    || echo "   checklist PDF skipped (Chrome not found)"
  rm -f "$HW/review/.review.html"
fi

grep -h 'ERC messages\|Found' "$HW/review/$BASE-erc.rpt" "$HW/review/$BASE-drc.rpt" | sed 's/^/   /'
echo "   outputs: $HW/{bom,fab,review}"
