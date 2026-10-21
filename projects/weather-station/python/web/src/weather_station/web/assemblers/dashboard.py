import math
from dataclasses import dataclass
from datetime import datetime, time, timedelta, timezone

from weather_station.shared.measurements import Measurement, StoredMeasurement
from weather_station.shared.registry import PropertyDefinition, SensorDefinition, get_sensor_definition
from weather_station.shared.contracts import MeasurementReader

MAX_CHART_POINTS = 2880
HISTORY_LIMIT = 50_000
CHART_WINDOW = timedelta(days=30)
GAP_STALE_MULTIPLE = 3
MIN_Y_TICKS = 4
MAX_Y_TICKS = 6
MAX_X_TICKS = 6
MAX_X_LABELS_ON_NARROW = 4
HOUR = timedelta(hours=1)
DAY = timedelta(days=1)
X_TICK_STEPS = (
    HOUR,
    3 * HOUR,
    6 * HOUR,
    12 * HOUR,
    DAY,
    2 * DAY,
    5 * DAY,
    7 * DAY,
)
VIEW_WIDTH = 900.0
VIEW_HEIGHT = 280.0
PLOT_LEFT = 8.0
PLOT_RIGHT = 16.0
PLOT_TOP = 32.0
PLOT_BOTTOM = 8.0


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
class ChartSegment:
    """A run of points with no data gap. A one-point segment is drawn as a marker at x, y."""

    line: str
    area: str
    is_single_point: bool
    x: float
    y: float


@dataclass(frozen=True)
class YTick:
    value: float
    label: str
    position: float
    coordinate: float
    percent: float


@dataclass(frozen=True)
class XTick:
    label: str
    position: float
    coordinate: float
    percent: float
    hidden_on_narrow: bool


@dataclass(frozen=True)
class TrendChart:
    """Tick position is 0..1 along the axis, from the axis minimum or the window start.

    Coordinates are in viewBox units, percents are offsets inside the SVG box.
    """

    points: tuple[ChartPoint, ...]
    segments: tuple[ChartSegment, ...]
    minimum: float | None
    maximum: float | None
    sample_count: int
    y_ticks: tuple[YTick, ...]
    x_ticks: tuple[XTick, ...]
    unit: str
    description: str
    width: float = VIEW_WIDTH
    height: float = VIEW_HEIGHT
    plot_left: float = PLOT_LEFT
    plot_right: float = VIEW_WIDTH - PLOT_RIGHT
    plot_top: float = PLOT_TOP
    plot_bottom: float = VIEW_HEIGHT - PLOT_BOTTOM

    @classmethod
    def empty(cls) -> "TrendChart":
        return cls((), (), None, None, 0, (), (), "", "")


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

        chart = TrendChart.empty()
        current_value = None
        if selected_sensor is not None and selected_property is not None:
            history = self._reader.series(
                selected_sensor.sensor_id,
                selected_property.property_id,
                now - CHART_WINDOW,
                limit=HISTORY_LIMIT,
            )
            chart = _build_chart(history, selected_property, now, self._stale_after)
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


def downsample(samples: list[ChartPoint], max_points: int) -> tuple[ChartPoint, ...]:
    """Averages time-ordered samples into max_points equal-duration buckets, skipping empty ones."""
    if len(samples) <= max_points:
        return tuple(samples)

    start = samples[0].timestamp
    bucket_duration = (samples[-1].timestamp - start) / max_points
    if not bucket_duration:
        return tuple(samples)

    buckets: dict[int, list[float]] = {}
    for sample in samples:
        index = min((sample.timestamp - start) // bucket_duration, max_points - 1)
        buckets.setdefault(index, []).append(sample.value)
    return tuple(
        ChartPoint(
            timestamp=start + bucket_duration * (index + 0.5),
            value=sum(values) / len(values),
        )
        for index, values in sorted(buckets.items())
    )


@dataclass(frozen=True)
class YScale:
    step: float
    decimals: int
    values: tuple[float, ...]


def y_scale(minimum: float, maximum: float) -> YScale:
    """Picks the smallest 1, 2 or 5 x 10^n step giving at most 6 ticks, widening the range outward to tick values.

    When the step leaves fewer than 4 ticks, extra ticks pad alternately below and above the range.
    """
    if maximum == minimum:
        span = max(abs(maximum) * 0.05, 1.0)
        minimum, maximum = minimum - span / 2, maximum + span / 2
    exponent = math.floor(math.log10((maximum - minimum) / MAX_Y_TICKS)) - 1
    while True:
        for multiplier in (1, 2, 5):
            step = multiplier * 10.0**exponent
            first = math.floor(minimum / step + 1e-9)
            last = math.ceil(maximum / step - 1e-9)
            if last - first + 1 <= MAX_Y_TICKS:
                while last - first + 1 < MIN_Y_TICKS:
                    if (last - first) % 2 == 0:
                        first -= 1
                    else:
                        last += 1
                decimals = max(0, -exponent)
                values = tuple(round(index * step, decimals) for index in range(first, last + 1))
                return YScale(step, decimals, values)
        exponent += 1


def x_tick_step(start: datetime, end: datetime) -> timedelta:
    """Smallest step in X_TICK_STEPS giving at most MAX_X_TICKS ticks between start and end."""
    return next(
        (step for step in X_TICK_STEPS if len(aligned_ticks(start, end, step)) <= MAX_X_TICKS),
        X_TICK_STEPS[-1],
    )


def aligned_ticks(start: datetime, end: datetime, step: timedelta) -> list[datetime]:
    """Ticks on local hours (step under a day) or local midnights (daily steps), within start..end.

    Each tick is built from local wall-clock time, so a DST change shifts the elapsed time
    between ticks and never their local alignment.
    """
    hours = range(0, 24, step // HOUR) if step < DAY else (0,)
    days_between = max(step // DAY, 1)
    ticks: set[datetime] = set()
    day = start.astimezone().date()
    last_day = end.astimezone().date()
    while day <= last_day:
        if day.toordinal() % days_between == 0:
            for hour in hours:
                moment = datetime.combine(day, time(hour)).astimezone()
                if start <= moment <= end:
                    ticks.add(moment)
        day += DAY
    return sorted(ticks)


def x_tick_labels(ticks: list[datetime], step: timedelta) -> list[str]:
    if step >= DAY:
        return [tick.strftime("%b %d") for tick in ticks]
    labels = []
    previous_day = None
    for tick in ticks:
        label = tick.strftime("%H:%M")
        if tick.date() != previous_day:
            label += tick.strftime("\n%b %d")
            previous_day = tick.date()
        labels.append(label)
    return labels


@dataclass(frozen=True)
class ChartFrame:
    """Maps time and value to viewBox coordinates inside the padded plot."""

    window_start: datetime
    window_end: datetime
    low: float
    high: float

    @property
    def plot_width(self) -> float:
        return VIEW_WIDTH - PLOT_LEFT - PLOT_RIGHT

    @property
    def plot_height(self) -> float:
        return VIEW_HEIGHT - PLOT_TOP - PLOT_BOTTOM

    def time_position(self, moment: datetime) -> float:
        return (moment - self.window_start) / (self.window_end - self.window_start)

    def value_position(self, value: float) -> float:
        return (value - self.low) / (self.high - self.low)

    def x(self, position: float) -> float:
        return PLOT_LEFT + position * self.plot_width

    def y(self, position: float) -> float:
        return PLOT_TOP + (1 - position) * self.plot_height

    def x_tick(self, moment: datetime, label: str, hidden_on_narrow: bool) -> XTick:
        position = self.time_position(moment)
        coordinate = self.x(position)
        return XTick(label, position, round(coordinate, 2), round(coordinate / VIEW_WIDTH * 100, 3), hidden_on_narrow)

    def y_tick(self, value: float, decimals: int) -> YTick:
        position = self.value_position(value)
        coordinate = self.y(position)
        return YTick(
            value,
            f"{value + 0.0:.{decimals}f}",
            position,
            round(coordinate, 2),
            round(coordinate / VIEW_HEIGHT * 100, 3),
        )

    def segment(self, points: list[ChartPoint]) -> ChartSegment:
        coordinates = [
            (self.x(self.time_position(point.timestamp)), self.y(self.value_position(point.value)))
            for point in points
        ]
        line = " ".join(f"{x:.1f},{y:.1f}" for x, y in coordinates)
        baseline = VIEW_HEIGHT - PLOT_BOTTOM
        first_x, last_x = coordinates[0][0], coordinates[-1][0]
        area = f"{first_x:.1f},{baseline:.1f} {line} {last_x:.1f},{baseline:.1f}"
        return ChartSegment(line, area, len(points) == 1, round(first_x, 1), round(coordinates[0][1], 1))


def split_at_gaps(points: tuple[ChartPoint, ...], threshold: timedelta) -> list[list[ChartPoint]]:
    runs = [[points[0]]]
    for previous, current in zip(points, points[1:]):
        if current.timestamp - previous.timestamp > threshold:
            runs.append([])
        runs[-1].append(current)
    return runs


def _describe(
    prop: PropertyDefinition,
    samples: list[ChartPoint],
    window_start: datetime,
    window_end: datetime,
) -> str:
    values = [sample.value for sample in samples]
    return (
        f"{prop.label} in {prop.display_unit}, {window_start:%b %d} to {window_end:%b %d}, "
        f"{len(samples)} readings. Low {min(values):.1f}, high {max(values):.1f}, "
        f"latest {values[-1]:.1f}."
    )


def _build_chart(
    history: list[StoredMeasurement],
    prop: PropertyDefinition,
    now: datetime,
    stale_after: timedelta,
) -> TrendChart:
    samples = [
        ChartPoint(
            timestamp=item.measurement.observed_at.astimezone(),
            value=prop.display_value(item.measurement.value),
        )
        for item in history
    ]
    if not samples:
        return TrendChart.empty()
    points = downsample(samples, MAX_CHART_POINTS)

    raw_values = [sample.value for sample in samples]
    minimum, maximum = min(raw_values), max(raw_values)
    scale = y_scale(minimum, maximum)
    window_end = max(now.astimezone(), samples[-1].timestamp)
    window_start = max(samples[0].timestamp, window_end - CHART_WINDOW)
    if window_start >= window_end:
        window_start = window_end - HOUR
    frame = ChartFrame(window_start, window_end, scale.values[0], scale.values[-1])

    bucket_duration = (
        (samples[-1].timestamp - samples[0].timestamp) / MAX_CHART_POINTS
        if len(samples) > MAX_CHART_POINTS
        else timedelta(0)
    )
    gap_threshold = max(GAP_STALE_MULTIPLE * stale_after, 2 * bucket_duration)
    segments = tuple(frame.segment(run) for run in split_at_gaps(points, gap_threshold))

    step = x_tick_step(window_start, window_end)
    ticks = aligned_ticks(window_start, window_end, step)
    hide_alternate = len(ticks) > MAX_X_LABELS_ON_NARROW
    x_ticks = tuple(
        frame.x_tick(tick, label, hide_alternate and index % 2 == 1)
        for index, (tick, label) in enumerate(zip(ticks, x_tick_labels(ticks, step)))
    )
    return TrendChart(
        points=points,
        segments=segments,
        minimum=minimum,
        maximum=maximum,
        sample_count=len(samples),
        y_ticks=tuple(frame.y_tick(value, scale.decimals) for value in scale.values),
        x_ticks=x_ticks,
        unit=prop.display_unit,
        description=_describe(prop, samples, window_start, window_end),
    )
