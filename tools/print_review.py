#!/usr/bin/env python3
"""Renders a board's review/REVIEW.md into a printable HTML page with empty checkboxes.

Usage: tools/print_review.py <REVIEW.md> <output.html>
Handles only what the review checklists use: headings, plain lines, checklist items, and tables.
"""
import html
import re
import sys

STYLE = """
body { font: 11pt -apple-system, Helvetica, Arial, sans-serif; margin: 0.6in; color: #000; }
h1 { font-size: 18pt; margin: 0 0 8pt; }
h2 { font-size: 13pt; margin: 16pt 0 6pt; border-bottom: 1px solid #000; page-break-after: avoid; }
p { margin: 3pt 0; }
ul { list-style: none; padding-left: 0; margin: 0; }
li { padding: 4pt 0 4pt 22pt; text-indent: -22pt; page-break-inside: avoid; }
li::before { content: "\\2610"; font-size: 13pt; display: inline-block; width: 22pt; text-indent: 0; }
code { font: 9.5pt Menlo, monospace; }
table { border-collapse: collapse; width: 100%; margin-top: 6pt; }
th, td { border: 1px solid #000; padding: 4pt; text-align: left; vertical-align: top; }
td { height: 30pt; }
h2:last-of-type { page-break-before: always; }
table { page-break-inside: avoid; }
"""


def inline(text):
    return re.sub(r"`([^`]+)`", r"<code>\1</code>", html.escape(text))


def render(markdown):
    out, in_list, in_table = [], False, False
    for line in markdown.splitlines():
        stripped = line.strip()
        if in_list and not stripped.startswith("- [ ]"):
            out.append("</ul>")
            in_list = False
        if in_table and not stripped.startswith("|"):
            out.append("</table>")
            in_table = False
        if not stripped:
            continue
        if stripped.startswith("#"):
            level = len(stripped) - len(stripped.lstrip("#"))
            out.append(f"<h{level}>{inline(stripped[level:].strip())}</h{level}>")
        elif stripped.startswith("- [ ]"):
            if not in_list:
                out.append("<ul>")
                in_list = True
            out.append(f"<li>{inline(stripped[5:].strip())}</li>")
        elif stripped.startswith("|"):
            cells = [c.strip() for c in stripped.strip("|").split("|")]
            if all(set(c) <= {"-"} for c in cells):
                continue
            tag = "th" if not in_table else "td"
            if not in_table:
                out.append("<table>")
                in_table = True
            out.append("<tr>" + "".join(f"<{tag}>{inline(c)}</{tag}>" for c in cells) + "</tr>")
        else:
            out.append(f"<p>{inline(stripped)}</p>")
    if in_list:
        out.append("</ul>")
    if in_table:
        out.append("</table>")
    return "\n".join(out)


source, target = sys.argv[1], sys.argv[2]
body = render(open(source, encoding="utf-8").read())
with open(target, "w", encoding="utf-8") as f:
    f.write(f"<!doctype html><html><head><meta charset='utf-8'><style>{STYLE}</style></head><body>{body}</body></html>")
