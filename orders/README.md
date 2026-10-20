# Orders

One folder per parts order placed with Digi-Key or Mouser. Each folder records what was bought, from
which supplier, and when.

## Folder name

`<yyyy-mm-dd>-<supplier>`, for example `2026-10-20-digikey`. The date is the day the order was
placed.

## Contents

| File | Contents |
| --- | --- |
| `order-bom.csv` | The merged BOM for every board in the order, grouped by MPN, with a per-board breakdown |

## Create an order

1. Regenerate each board's BOM: `tools/release.sh projects/<name>/hw`
2. Merge them: `tools/combine_boms.py orders/<yyyy-mm-dd>-<supplier>/order-bom.csv`
3. Upload `order-bom.csv` to the supplier's BOM tool. Both Digi-Key and Mouser match on MPN.
4. Commit the folder once the order is placed.
