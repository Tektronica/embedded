from dataclasses import dataclass
from datetime import datetime, timedelta, timezone

from weather_station.shared.measurements import Measurement, StoredMeasurement
from weather_station.shared.registry import PropertyDefinition, SensorDefinition, get_sensor_definition
from weather_station.shared.contracts import MeasurementReader


@dataclass(frozen=True)
class SensorSummary:
    sensor_id: str
    sensor_type: str
    label: str
    latest_at: datetime | None
    is_stale: bool


@dataclass(frozen=True)
class ChartPoint:
    timestamp: datetime
    value: float


@dataclass(frozen=True)
class TrendChart:
    points: tuple[ChartPoint, ...]
    line: str
    area: str
    minimum: float | None
    maximum: float | None


@dataclass(frozen=True)
class DashboardView:
    status: str
    last_update: datetime | None
    sensors: tuple[SensorSummary, ...]
    selected_sensor: SensorSummary | None
    definition: SensorDefinition | None
    properties: tuple[PropertyDefinition, ...]
    selected_property: PropertyDefinition | None
    selected_measurement: Measurement | None
    current_value: float | None
    chart: TrendChart


class DashboardAssembler:
    """Builds the dashboard view. Timestamps come out in the server's local time zone."""

    def __init__(self, reader: MeasurementReader, stale_after: timedelta) -> None:
        self._reader = reader
        self._stale_after = stale_after

    def assemble(
        self,
        sensor_id: str | None,
        property_key: str | None,
    ) -> DashboardView:
        now = datetime.now(timezone.utc)
        latest = self._reader.latest()
        registered_sensors = self._reader.registered_sensors()
        latest_by_sensor = {
            registered_id: max(
                (
                    item.measurement.observed_at
                    for item in latest
                    if item.measurement.sensor_id == registered_id
                ),
                default=None,
            )
            for registered_id, _ in registered_sensors
        }
        sensors = tuple(
            SensorSummary(
                sensor_id=registered_id,
                sensor_type=sensor_type,
                label=get_sensor_definition(sensor_type).label,
                latest_at=_local(latest_by_sensor[registered_id]),
                is_stale=_is_stale(latest_by_sensor[registered_id], now, self._stale_after),
            )
            for registered_id, sensor_type in registered_sensors
        )
        selected_sensor = next(
            (item for item in sensors if item.sensor_id == sensor_id),
            sensors[0] if sensors else None,
        )
        definition = (
            get_sensor_definition(selected_sensor.sensor_type)
            if selected_sensor is not None
            else None
        )
        properties = definition.properties if definition is not None else ()
        selected_property = next(
            (item for item in properties if item.key == property_key),
            properties[0] if properties else None,
        )
        latest_measurement = next(
            (
                item.measurement
                for item in latest
                if selected_sensor is not None
                and selected_property is not None
                and item.measurement.sensor_id == selected_sensor.sensor_id
                and item.measurement.property_id == selected_property.property_id
            ),
            None,
        )

        chart = TrendChart((), "", "", None, None)
        current_value = None
        if selected_sensor is not None and selected_property is not None:
            history = self._reader.series(
                selected_sensor.sensor_id,
                selected_property.property_id,
                datetime.now(timezone.utc) - timedelta(days=30),
            )
            chart = _build_chart(history, selected_property)
            if latest_measurement is not None:
                current_value = selected_property.display_value(latest_measurement.value)

        reported = [item.latest_at for item in sensors if item.latest_at is not None]
        last_update = max(reported, default=None)
        return DashboardView(
            status=_status(sensors, last_update),
            last_update=last_update,
            sensors=sensors,
            selected_sensor=selected_sensor,
            definition=definition,
            properties=properties,
            selected_property=selected_property,
            selected_measurement=latest_measurement,
            current_value=current_value,
            chart=chart,
        )


def _local(moment: datetime | None) -> datetime | None:
    return moment.astimezone() if moment is not None else None


def _is_stale(latest_at: datetime | None, now: datetime, stale_after: timedelta) -> bool:
    return latest_at is not None and now - latest_at > stale_after


def _status(sensors: tuple[SensorSummary, ...], last_update: datetime | None) -> str:
    if last_update is None:
        return "no-data"
    return "stale" if any(item.is_stale for item in sensors) else "live"


def _build_chart(
    history: list[StoredMeasurement],
    prop: PropertyDefinition,
) -> TrendChart:
    points = tuple(
        ChartPoint(
            timestamp=item.measurement.observed_at.astimezone(),
            value=prop.display_value(item.measurement.value),
        )
        for item in history
    )
    if not points:
        return TrendChart((), "", "", None, None)

    width, height = 900.0, 280.0
    left, right, top, bottom = 48.0, 16.0, 16.0, 32.0
    values = [point.value for point in points]
    minimum, maximum = min(values), max(values)
    span = maximum - minimum or max(abs(maximum) * 0.05, 1.0)
    coordinates = [
        (
            left + index * (width - left - right) / max(len(points) - 1, 1),
            top + (maximum - value) * (height - top - bottom) / span,
        )
        for index, value in enumerate(values)
    ]
    line = " ".join(f"{x:.1f},{y:.1f}" for x, y in coordinates)
    first_x, last_x = coordinates[0][0], coordinates[-1][0]
    baseline = height - bottom
    area = f"{first_x:.1f},{baseline:.1f} {line} {last_x:.1f},{baseline:.1f}"
    return TrendChart(points, line, area, minimum, maximum)
