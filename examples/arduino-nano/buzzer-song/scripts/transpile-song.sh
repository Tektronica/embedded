#!/usr/bin/env bash
set -euo pipefail

# Transpiles a robsoncouto/arduino-songs-style Arduino sketch into our song::Note pair format.
# See ../docs/transpiling-songs.md for the full walkthrough of what this does and why.
#
# Usage: scripts/transpile-song.sh <source.ino> [notes-per-line]
#
# Prints the transformed `{notes::X, N},` pairs to stdout, grouped `notes-per-line` (default 8)
# per line -- ready to paste into a new include/songs/<Song>.h's NOTES[] array. Prints a note
# count to stderr; cross-check it against the source file's own
# `notes = sizeof(melody) / sizeof(melody[0]) / 2` comment/line before trusting the output.

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <source.ino> [notes-per-line]" >&2
  exit 1
fi

source_file="$1"
notes_per_line="${2:-8}"

if [[ ! -f "$source_file" ]]; then
  echo "error: '$source_file' not found" >&2
  exit 1
fi

# 1. Pull out just the melody array body (works for both `int melody[]` and the
#    `const int melody[] PROGMEM` form Doom's source uses).
# 2. Strip trailing `// measure` comments -- harmless to remove, and some source files
#    (Doom's) put the note and its duration on separate lines, which the comment-stripping
#    step must happen before joining, or a comment can swallow the next line's token.
# 3. Join everything into one stream so note/duration pairs are adjacent regardless of the
#    source's original line breaks (one-per-line vs eight-per-line both normalize the same way).
# 4. Rename tokens into our namespace: NOTE_ -> notes::, then lowercase the sharp letter
#    (GS5 -> Gs5, matching Notes.h's `Gs5` constant naming), then bare REST -> notes::REST.
# 5. Wrap each (pitch, duration) pair in braces, one pair per output line.
pairs=$(
  awk '/melody\[\] *(PROGMEM)? *= *\{/{flag=1; next} /^\};/{flag=0} flag' "$source_file" \
    | sed -E 's|//.*||' \
    | tr '\n' ' ' \
    | sed -E 's/NOTE_/notes::/g; s/::([A-G])S([0-9])/::\1s\2/g; s/REST/notes::REST/g' \
    | sed -E 's/(notes::[A-Za-z0-9]+), *(-?[0-9]+),?/{\1, \2},\n/g'
)

count=$(printf '%s\n' "$pairs" | grep -c 'notes::' || true)

# Regroup from one-pair-per-line into `notes_per_line` pairs per line, for readability against
# the eight-per-line style existing songs/*.h files use.
printf '%s\n' "$pairs" | awk -v n="$notes_per_line" '
  { gsub(/^[ \t]+/, ""); printf "    %s ", $0 }
  NR % n == 0 { print "" }
  END { if (NR % n != 0) print "" }
'

echo "-- $count notes transcribed from $source_file --" >&2
echo "-- cross-check this against the source file's own melody/notes-count comment --" >&2
