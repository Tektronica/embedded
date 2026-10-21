from collections.abc import Mapping, Sequence
from dataclasses import dataclass
from datetime import datetime, timedelta
from enum import StrEnum

from weather_station.notifications.alarm_rules import AlarmRule
from weather_station.notifications.alarm_state import AlarmState
from weather_station.notifications.contracts import Notification

ABOVE = "above"
BELOW = "below"


class ThresholdEventKind(StrEnum):
    ABOVE = "above"
    BELOW = "below"
    BACK_BELOW = "back below"
    BACK_ABOVE = "back above"


class FreshnessEventKind(StrEnum):
    STALE = "stale"
    REPORTING = "reporting"


@dataclass(frozen=True)
class AlarmReading:
    label: str
    unit: str
    value: float
    observed_at: datetime


@dataclass(frozen=True)
class SensorSnapshot:
    last_observed_at: datetime
    readings: Mapping[str, AlarmReading]


@dataclass(frozen=True)
class ThresholdEvent:
    kind: ThresholdEventKind
    sensor: str
    limit: float
    reading: AlarmReading


@dataclass(frozen=True)
class FreshnessEvent:
    kind: FreshnessEventKind
    sensor: str
    last_observed_at: datetime | None


AlarmEvent = ThresholdEvent | FreshnessEvent


def evaluate_alarms(
    rules: Sequence[AlarmRule],
    snapshots: Mapping[str, SensorSnapshot],
    previous: AlarmState,
    now: datetime,
    stale_after: timedelta,
) -> tuple[AlarmState, list[AlarmEvent]]:
    events: list[AlarmEvent] = []
    stale_sensors: set[str] = set()
    for sensor in sorted({rule.sensor for rule in rules}):
        snapshot = snapshots.get(sensor)
        is_stale = snapshot is None or now - snapshot.last_observed_at > stale_after
        was_stale = sensor in previous.stale
        last_observed_at = snapshot.last_observed_at if snapshot else None
        if is_stale:
            stale_sensors.add(sensor)
        if is_stale and not was_stale:
            events.append(FreshnessEvent(FreshnessEventKind.STALE, sensor, last_observed_at))
        if was_stale and not is_stale:
            events.append(FreshnessEvent(FreshnessEventKind.REPORTING, sensor, last_observed_at))

    thresholds: dict[str, frozenset[str]] = {}
    for rule in rules:
        before = previous.thresholds.get(rule.key, frozenset())
        reading = _fresh_reading(rule, snapshots, stale_sensors)
        after = before if reading is None else _next_sides(rule, before, reading.value)
        if reading is not None:
            events.extend(_threshold_events(rule, before, after, reading))
        if after:
            thresholds[rule.key] = after
    return AlarmState(thresholds, frozenset(stale_sensors)), events


def format_notification(events: Sequence[AlarmEvent], now: datetime) -> Notification:
    subject = "Weather station alarm"
    if len(events) > 1:
        subject = f"{subject}: {len(events)} events"
    body = "\n".join(_format_event(event, now) for event in events)
    return Notification(subject=subject, body=f"{body}\n")


def _fresh_reading(
    rule: AlarmRule,
    snapshots: Mapping[str, SensorSnapshot],
    stale_sensors: set[str],
) -> AlarmReading | None:
    snapshot = snapshots.get(rule.sensor)
    if snapshot is None or rule.sensor in stale_sensors:
        return None
    return snapshot.readings.get(rule.property_key)


def _next_sides(rule: AlarmRule, before: frozenset[str], value: float) -> frozenset[str]:
    sides = set(before)
    if rule.above is not None:
        if value > rule.above:
            sides.add(ABOVE)
        elif value < rule.above - rule.hysteresis:
            sides.discard(ABOVE)
    if rule.below is not None:
        if value < rule.below:
            sides.add(BELOW)
        elif value > rule.below + rule.hysteresis:
            sides.discard(BELOW)
    return frozenset(sides)


def _threshold_events(
    rule: AlarmRule,
    before: frozenset[str],
    after: frozenset[str],
    reading: AlarmReading,
) -> list[ThresholdEvent]:
    events: list[ThresholdEvent] = []
    if rule.above is not None:
        if ABOVE in after and ABOVE not in before:
            events.append(ThresholdEvent(ThresholdEventKind.ABOVE, rule.sensor, rule.above, reading))
        if ABOVE in before and ABOVE not in after:
            clear_limit = rule.above - rule.hysteresis
            events.append(ThresholdEvent(ThresholdEventKind.BACK_BELOW, rule.sensor, clear_limit, reading))
    if rule.below is not None:
        if BELOW in after and BELOW not in before:
            events.append(ThresholdEvent(ThresholdEventKind.BELOW, rule.sensor, rule.below, reading))
        if BELOW in before and BELOW not in after:
            clear_limit = rule.below + rule.hysteresis
            events.append(ThresholdEvent(ThresholdEventKind.BACK_ABOVE, rule.sensor, clear_limit, reading))
    return events


def _format_event(event: AlarmEvent, now: datetime) -> str:
    if isinstance(event, ThresholdEvent):
        reading = event.reading
        unit = f" {reading.unit}" if reading.unit else ""
        return (
            f"{event.sensor} {reading.label.lower()} {event.kind.value} {event.limit:.1f}{unit}: "
            f"now {reading.value:.1f}{unit} ({_format_time(reading.observed_at, now)})"
        )
    if event.kind is FreshnessEventKind.STALE:
        if event.last_observed_at is None:
            return f"{event.sensor} stale: no readings yet"
        return f"{event.sensor} stale: no reading since {_format_time(event.last_observed_at, now)}"
    if event.last_observed_at is None:
        return f"{event.sensor} reporting again"
    return f"{event.sensor} reporting again: last reading {_format_time(event.last_observed_at, now)}"


def _format_time(moment: datetime, now: datetime) -> str:
    return f"{moment.astimezone(now.tzinfo):%Y-%m-%d %H:%M %Z}"
