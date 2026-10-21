from dataclasses import dataclass
from datetime import date, datetime, time, timedelta, timezone

from weather_station.notifications.contracts import Notification
from weather_station.shared.contracts import MeasurementReader
from weather_station.shared.registry import get_property_definition, get_sensor_definition
from weather_station.shared.units import UnitSystem

DIGEST_WINDOW = timedelta(hours=24)
DAILY_TABLE_DAYS = 7
YESTERDAY_TOLERANCE = timedelta(minutes=30)
SERIES_LIMIT = 50_000


@dataclass(frozen=True)
class Reading:
    observed_at: datetime
    value: float


@dataclass(frozen=True)
class DailyRange:
    day: date
    high: float
    low: float


@dataclass(frozen=True)
class PropertySummary:
    label: str
    unit: str
    latest: float
    observed_at: datetime
    minimum: float
    mean: float
    maximum: float
    change_since_yesterday: float | None
    daily_ranges: tuple[DailyRange, ...]


class DigestBuilder:
    """Builds one summary per sensor: latest reading, 24 hour stats, change since yesterday, and daily highs and lows."""

    def __init__(self, reader: MeasurementReader, units: UnitSystem) -> None:
        self._reader = reader
        self._units = units

    def build(self, now: datetime) -> Notification:
        sections: list[str] = []
        for sensor_id, sensor_type in self._reader.registered_sensors():
            summaries = self._summaries(sensor_id, sensor_type, now)
            label = get_sensor_definition(sensor_type).label
            sections.append(_format_section(sensor_id, label, summaries))
        body = "\n\n".join(sections) if sections else "No sensors have reported yet."
        return Notification(
            subject=f"Weather station summary for {now:%Y-%m-%d}",
            body=f"Readings for the 24 hours ending {now:%Y-%m-%d %H:%M %Z}.\n\n{body}\n",
        )

    def _summaries(self, sensor_id: str, sensor_type: str, now: datetime) -> list[PropertySummary]:
        table_start = _table_start(now)
        window_start = now.astimezone(timezone.utc) - DIGEST_WINDOW
        summaries: list[PropertySummary] = []
        for stored in self._reader.latest_for_sensor(sensor_id):
            prop = get_property_definition(sensor_type, stored.measurement.property_id)
            history = [
                Reading(item.measurement.observed_at, self._units.value(prop, item.measurement.value))
                for item in self._reader.series(sensor_id, prop.property_id, table_start, SERIES_LIMIT)
            ]
            latest = Reading(
                stored.measurement.observed_at, self._units.value(prop, stored.measurement.value)
            )
            last_day = [item.value for item in history if item.observed_at >= window_start]
            summaries.append(
                PropertySummary(
                    label=prop.label,
                    unit=self._units.unit(prop),
                    latest=latest.value,
                    observed_at=latest.observed_at.astimezone(now.tzinfo),
                    minimum=min(last_day, default=latest.value),
                    mean=sum(last_day) / len(last_day) if last_day else latest.value,
                    maximum=max(last_day, default=latest.value),
                    change_since_yesterday=_change_since_yesterday(latest, history),
                    daily_ranges=_daily_ranges(history, now),
                )
            )
        return summaries


def _table_start(now: datetime) -> datetime:
    first_day = now.date() - timedelta(days=DAILY_TABLE_DAYS - 1)
    return datetime.combine(first_day, time.min, tzinfo=now.tzinfo)


def _change_since_yesterday(latest: Reading, history: list[Reading]) -> float | None:
    target = latest.observed_at.astimezone(timezone.utc) - DIGEST_WINDOW
    nearby = [item for item in history if abs(item.observed_at - target) <= YESTERDAY_TOLERANCE]
    if not nearby:
        return None
    closest = min(nearby, key=lambda item: abs(item.observed_at - target))
    return latest.value - closest.value


def _daily_ranges(history: list[Reading], now: datetime) -> tuple[DailyRange, ...]:
    values_by_day: dict[date, list[float]] = {}
    for item in history:
        day = item.observed_at.astimezone(now.tzinfo).date()
        values_by_day.setdefault(day, []).append(item.value)
    return tuple(
        DailyRange(day, high=max(values), low=min(values))
        for day, values in sorted(values_by_day.items())
    )


def _format_section(sensor_id: str, label: str, summaries: list[PropertySummary]) -> str:
    if not summaries:
        return f"{label} ({sensor_id})\n  No readings yet."
    lines = [f"{label} ({sensor_id})"]
    for item in summaries:
        lines.extend(_format_property(item))
    return "\n".join(lines)


def _format_property(item: PropertySummary) -> list[str]:
    unit = f" {item.unit}" if item.unit else ""
    lines = [
        f"  {item.label}",
        f"    Latest:          {item.latest:.1f}{unit} ({item.observed_at:%Y-%m-%d %H:%M %Z})",
        f"    Last 24 h:       min {item.minimum:.1f}, mean {item.mean:.1f}, max {item.maximum:.1f}{unit}",
    ]
    if item.change_since_yesterday is not None:
        lines.append(f"    Since yesterday: {item.change_since_yesterday:+.1f}{unit}")
    if item.daily_ranges:
        lines.append("    Daily high and low:")
        for daily in item.daily_ranges:
            lines.append(
                f"      {daily.day:%Y-%m-%d}  high {daily.high:6.1f}  low {daily.low:6.1f}{unit}"
            )
    return lines
