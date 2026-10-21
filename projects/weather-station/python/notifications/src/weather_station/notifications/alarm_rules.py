import math
from dataclasses import dataclass
from typing import Any

from weather_station.notifications.errors import InvalidAlarmConfiguration
from weather_station.shared.registry import SENSOR_DEFINITIONS

ALARM_KEYS = frozenset({"sensor", "property", "above", "below", "hysteresis"})
KNOWN_PROPERTY_KEYS = frozenset(
    item.key for definition in SENSOR_DEFINITIONS.values() for item in definition.properties
)


@dataclass(frozen=True)
class AlarmRule:
    sensor: str
    property_key: str
    above: float | None
    below: float | None
    hysteresis: float

    @property
    def key(self) -> str:
        return f"{self.sensor}/{self.property_key}"


def read_alarm_rules(document: dict[str, Any]) -> list[AlarmRule]:
    entries = document.get("alarms", [])
    if not isinstance(entries, list):
        raise InvalidAlarmConfiguration("Config section 'alarms' must be an array")
    rules = [_read_rule(index, entry) for index, entry in enumerate(entries)]
    seen: set[str] = set()
    for rule in rules:
        if rule.key in seen:
            raise InvalidAlarmConfiguration(f"Config section 'alarms' has a duplicate rule for {rule.key}")
        seen.add(rule.key)
    return rules


def _read_rule(index: int, entry: Any) -> AlarmRule:
    name = f"alarms[{index}]"
    if not isinstance(entry, dict):
        raise InvalidAlarmConfiguration(f"Config key '{name}' must be an object")
    unknown = set(entry) - ALARM_KEYS
    if unknown:
        raise InvalidAlarmConfiguration(f"Config key '{name}' has unknown keys: {', '.join(sorted(unknown))}")
    sensor = _read_text(entry, name, "sensor")
    property_key = _read_text(entry, name, "property")
    if property_key not in KNOWN_PROPERTY_KEYS:
        raise InvalidAlarmConfiguration(f"Config key '{name}.property' is not a known property: {property_key}")
    above = _read_optional_number(entry, name, "above")
    below = _read_optional_number(entry, name, "below")
    if above is None and below is None:
        raise InvalidAlarmConfiguration(f"Config key '{name}' needs 'above' or 'below'")
    hysteresis = _read_optional_number(entry, name, "hysteresis") or 0.0
    if hysteresis < 0:
        raise InvalidAlarmConfiguration(f"Config key '{name}.hysteresis' must not be negative")
    if above is not None and below is not None and above - below <= 2 * hysteresis:
        raise InvalidAlarmConfiguration(
            f"Config key '{name}' needs 'above' minus 'below' to exceed twice 'hysteresis'"
        )
    return AlarmRule(sensor, property_key, above, below, hysteresis)


def _read_text(entry: dict[str, Any], name: str, key: str) -> str:
    value = entry.get(key)
    if not isinstance(value, str) or not value:
        raise InvalidAlarmConfiguration(f"Config key '{name}.{key}' must be a non-empty string")
    return value


def _read_optional_number(entry: dict[str, Any], name: str, key: str) -> float | None:
    if key not in entry:
        return None
    value = entry[key]
    if not isinstance(value, (int, float)) or isinstance(value, bool) or not math.isfinite(value):
        raise InvalidAlarmConfiguration(f"Config key '{name}.{key}' must be a number")
    return float(value)
