# lib/ToneSequence

Hardware-free, non-blocking tone sequencing. Header-only, no dependencies beyond `stdint.h`.

- `tonesequence::Step{frequencyHz, durationMs}`: one step; `SILENCE` for a gap.
- `tonesequence::Sequence{steps, count, loops}`: a sound, played once or looped.
- `toneStateAt(sequence, elapsedMs)`: the `ToneState{on, frequencyHz}` to play right now.
- `isFinished(sequence, elapsedMs)`: true once a one-shot sequence ends; never for a looped one.

The app owns the sound tables and maps `ToneState` to `tone()`/`noTone()` each loop.

## Use from a project

```ini
lib_deps = symlink://<path to repo root>/lib/ToneSequence
```

## Test

```bash
pio test -d lib/ToneSequence
```
