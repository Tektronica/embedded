import json
import logging
import os
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from weather_station.notifications.errors import InvalidAlarmState

SIDES = frozenset({"above", "below"})


@dataclass(frozen=True)
class AlarmState:
    thresholds: dict[str, frozenset[str]]
    stale: frozenset[str]

    @classmethod
    def empty(cls) -> "AlarmState":
        return cls(thresholds={}, stale=frozenset())


class AlarmStateStore:
    """Keeps the alarm state in one JSON file. Writes go through a temp file so a crash never leaves a partial file."""

    def __init__(self, path: Path) -> None:
        self._path = path

    def load(self) -> AlarmState:
        try:
            document = json.loads(self._path.read_text(encoding="utf-8"))
            return _parse_state(document)
        except FileNotFoundError:
            logging.warning("No alarm state at %s, starting empty", self._path)
        except (ValueError, InvalidAlarmState):
            logging.warning("Alarm state at %s is unreadable, starting empty", self._path, exc_info=True)
        return AlarmState.empty()

    def save(self, state: AlarmState) -> None:
        document = {
            "thresholds": {key: sorted(sides) for key, sides in sorted(state.thresholds.items())},
            "stale": sorted(state.stale),
        }
        self._path.parent.mkdir(parents=True, exist_ok=True)
        temporary = self._path.with_name(f"{self._path.name}.tmp")
        temporary.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
        os.replace(temporary, self._path)


def _parse_state(document: Any) -> AlarmState:
    if not isinstance(document, dict) or set(document) != {"thresholds", "stale"}:
        raise InvalidAlarmState("Alarm state must be an object with 'thresholds' and 'stale'")
    thresholds = document["thresholds"]
    stale = document["stale"]
    if not isinstance(thresholds, dict) or not isinstance(stale, list):
        raise InvalidAlarmState("Alarm state 'thresholds' must be an object and 'stale' an array")
    if not all(isinstance(sensor, str) for sensor in stale):
        raise InvalidAlarmState("Alarm state 'stale' must hold strings")
    parsed: dict[str, frozenset[str]] = {}
    for key, sides in thresholds.items():
        if not isinstance(sides, list) or not all(side in SIDES for side in sides):
            raise InvalidAlarmState(f"Alarm state sides for {key!r} must be a list of 'above' or 'below'")
        parsed[key] = frozenset(sides)
    return AlarmState(thresholds=parsed, stale=frozenset(stale))
