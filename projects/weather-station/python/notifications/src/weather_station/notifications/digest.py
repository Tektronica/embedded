from dataclasses import dataclass
from datetime import datetime, timedelta

from weather_station.notifications.contracts import Notification
from weather_station.shared.contracts import MeasurementReader
from weather_station.shared.registry import get_property_definition, get_sensor_definition

DIGEST_WINDOW = timedelta(hours=24)


@dataclass(frozen=True)
class PropertySummary:
    label: str
    unit: str
    latest: float
    minimum: float
    maximum: float
    observed_at: datetime


class DigestBuilder:
    """Builds one summary of every sensor's latest reading and its range over the last 24 hours."""

    def __init__(self, reader: MeasurementReader) -> None:
        self._reader = reader

    def build(self, now: datetime) -> Notification:
        since = now - DIGEST_WINDOW
        sections: list[str] = []
        for sensor_id, sensor_type in self._reader.registered_sensors():
            summaries = self._summaries(sensor_id, sensor_type, since)
            label = get_sensor_definition(sensor_type).label
            sections.append(_format_section(sensor_id, label, summaries))
        body = "\n\n".join(sections) if sections else "No sensors have reported yet."
        return Notification(
            subject=f"Weather station summary for {now:%Y-%m-%d}",
            body=f"Readings for the 24 hours ending {now:%Y-%m-%d %H:%M %Z}.\n\n{body}\n",
        )

    def _summaries(self, sensor_id: str, sensor_type: str, since: datetime) -> list[PropertySummary]:
        summaries: list[PropertySummary] = []
        for stored in self._reader.latest_for_sensor(sensor_id):
            prop = get_property_definition(sensor_type, stored.measurement.property_id)
            history = self._reader.series(sensor_id, prop.property_id, since)
            values = [prop.display_value(item.measurement.value) for item in history]
            latest = prop.display_value(stored.measurement.value)
            summaries.append(
                PropertySummary(
                    label=prop.label,
                    unit=prop.display_unit,
                    latest=latest,
                    minimum=min(values, default=latest),
                    maximum=max(values, default=latest),
                    observed_at=stored.measurement.observed_at,
                )
            )
        return summaries


def _format_section(sensor_id: str, label: str, summaries: list[PropertySummary]) -> str:
    if not summaries:
        return f"{label} ({sensor_id})\n  No readings yet."
    lines = [f"{label} ({sensor_id})"]
    for item in summaries:
        unit = f" {item.unit}" if item.unit else ""
        lines.append(
            f"  {item.label}: {item.latest:.1f}{unit}"
            f" (24 h range {item.minimum:.1f} to {item.maximum:.1f}{unit},"
            f" last reading {item.observed_at:%Y-%m-%d %H:%M} UTC)"
        )
    return "\n".join(lines)
