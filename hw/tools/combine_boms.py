#!/usr/bin/env python3
"""Merges every projects/<name>/hw/bom/*-bom.csv into one order list grouped by MPN.

Usage: hw/tools/combine_boms.py [output.csv]   (writes to stdout when no path is given)
"""
import csv
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
COLUMNS = ["MPN", "Manufacturer", "Digi-Key", "Mouser", "LCSC", "Value", "Footprint", "Qty", "Boards"]

merged = {}
for bom in sorted(REPO.glob("projects/*/hw/bom/*-bom.csv")):
    board = bom.name.removesuffix("-bom.csv")
    for row in csv.DictReader(bom.open(newline="", encoding="utf-8")):
        key = row["MPN"] or f'{row["Value"]}|{row["Footprint"]}'
        line = merged.setdefault(key, {c: row.get(c, "") for c in COLUMNS[:7]} | {"Qty": 0, "Boards": []})
        line["Qty"] += int(row["Qty"])
        line["Boards"].append(f'{board} x{row["Qty"]} ({row["Reference"]})')

out = open(sys.argv[1], "w", newline="", encoding="utf-8") if len(sys.argv) > 1 else sys.stdout
writer = csv.DictWriter(out, fieldnames=COLUMNS, lineterminator="\n")
writer.writeheader()
for line in sorted(merged.values(), key=lambda r: (r["MPN"] == "", r["MPN"], r["Value"])):
    writer.writerow(line | {"Boards": "; ".join(line["Boards"])})
