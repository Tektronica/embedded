import os
import time
import unittest
from datetime import datetime, timedelta, timezone
from uuid import uuid4

from weather_station.shared.measurements import Measurement, MeasurementQuality, StoredMeasurement
from weather_station.shared.registry import TEMPERATURE
from weather_station.web.assemblers.dashboard import (
    DAY,
    HOUR,
    MAX_CHART_POINTS,
    PLOT_LEFT,
    PLOT_RIGHT,
    VIEW_WIDTH,
    ChartPoint,
    TrendChart,
    _build_chart,
    aligned_ticks,
    split_at_gaps,
    x_tick_labels,
    x_tick_step,
    y_scale,
)

NOW = datetime(2026, 10, 21, 12, 0, tzinfo=timezone.utc)
STALE_AFTER = timedelta(minutes=5)
PLOT_RIGHT_EDGE = VIEW_WIDTH - PLOT_RIGHT


def stored(observed_at: datetime, kelvin: float) -> StoredMeasurement:
    return StoredMeasurement(
        "temperature_humidity",
        Measurement(
            id=uuid4(),
            created_at=observed_at,
            observed_at=observed_at,
            sensor_id="room-am2302",
            property_id=TEMPERATURE.property_id,
            value=kelvin,
            unit=TEMPERATURE.si_unit,
            quality=MeasurementQuality.VALID,
            uncertainty=None,
        ),
    )


class NewYorkTimeTestCase(unittest.TestCase):
    def setUp(self) -> None:
        self.previous_zone = os.environ.get("TZ")
        os.environ["TZ"] = "America/New_York"
        time.tzset()

    def tearDown(self) -> None:
        if self.previous_zone is None:
            del os.environ["TZ"]
        else:
            os.environ["TZ"] = self.previous_zone
        time.tzset()


class YScaleTests(unittest.TestCase):
    def test_steps_are_one_two_or_five_times_a_power_of_ten(self) -> None:
        cases = [
            (0.0, 10.0, 2.0),
            (0.0, 100.0, 20.0),
            (3.2, 7.9, 1.0),
            (0.0, 1.0, 0.2),
            (18.3, 24.7, 2.0),
            (1002.0, 1031.0, 10.0),
            (-12.0, -3.0, 2.0),
            (-4.0, 6.0, 2.0),
            (0.0, 0.03, 0.01),
        ]

        for minimum, maximum, step in cases:
            with self.subTest(minimum=minimum, maximum=maximum):
                self.assertAlmostEqual(y_scale(minimum, maximum).step, step)

    def test_tick_count_stays_between_four_and_six(self) -> None:
        for low_tenths in range(-300, 300, 7):
            for span_tenths in (1, 3, 7, 12, 25, 49, 100, 333, 1000, 7777):
                low, high = low_tenths / 10, low_tenths / 10 + span_tenths / 10
                with self.subTest(low=low, high=high):
                    self.assertIn(len(y_scale(low, high).values), range(4, 7))

    def test_range_widens_to_the_nearest_ticks_outside_the_data(self) -> None:
        scale = y_scale(18.3, 24.7)

        self.assertEqual(scale.values, (18.0, 20.0, 22.0, 24.0, 26.0))

    def test_negative_range_widens_and_labels_use_the_step_decimals(self) -> None:
        scale = y_scale(-12.4, -3.1)

        self.assertEqual(scale.values, (-15.0, -10.0, -5.0, 0.0))
        self.assertEqual(scale.decimals, 0)

    def test_range_leaving_fewer_than_four_ticks_is_padded(self) -> None:
        scale = y_scale(5.0, 15.0)

        self.assertEqual(scale.values, (0.0, 5.0, 10.0, 15.0))

    def test_fractional_step_gives_decimals(self) -> None:
        scale = y_scale(0.31, 0.88)

        self.assertEqual(scale.decimals, 1)
        self.assertEqual(scale.values, (0.2, 0.4, 0.6, 0.8, 1.0))

    def test_flat_data_uses_the_fallback_span(self) -> None:
        scale = y_scale(22.0, 22.0)

        self.assertLess(scale.values[0], 22.0)
        self.assertGreater(scale.values[-1], 22.0)
        self.assertIn(len(scale.values), range(4, 7))

    def test_flat_zero_data_has_ticks_around_zero(self) -> None:
        scale = y_scale(0.0, 0.0)

        self.assertIn(0.0, scale.values)
        self.assertLess(scale.values[0], 0.0)


class XTickTests(NewYorkTimeTestCase):
    def test_six_hour_span_uses_hourly_ticks(self) -> None:
        end = datetime(2026, 10, 21, 16, 30, tzinfo=timezone.utc)

        self.assertEqual(x_tick_step(end - 6 * HOUR, end), HOUR)

    def test_two_day_span_uses_twelve_hour_ticks(self) -> None:
        end = datetime(2026, 10, 21, 16, 30, tzinfo=timezone.utc)

        self.assertEqual(x_tick_step(end - 2 * DAY, end), 12 * HOUR)

    def test_thirty_day_span_uses_five_day_ticks(self) -> None:
        end = datetime(2026, 10, 21, 16, 30, tzinfo=timezone.utc)

        step = x_tick_step(end - 30 * DAY, end)

        self.assertEqual(step, 5 * DAY)
        self.assertLessEqual(len(aligned_ticks(end - 30 * DAY, end, step)), 6)

    def test_ticks_fall_on_local_hours(self) -> None:
        end = datetime(2026, 10, 21, 16, 30, tzinfo=timezone.utc)

        ticks = aligned_ticks(end - 2 * DAY, end, 12 * HOUR)

        self.assertEqual({tick.hour for tick in ticks}, {0, 12})
        self.assertTrue(all(tick.minute == 0 for tick in ticks))

    def test_spring_forward_day_keeps_local_alignment(self) -> None:
        start = datetime(2026, 3, 7, 17, 0, tzinfo=timezone.utc)
        end = datetime(2026, 3, 9, 17, 0, tzinfo=timezone.utc)

        ticks = aligned_ticks(start, end, 6 * HOUR)

        self.assertEqual({tick.hour for tick in ticks}, {0, 6, 12, 18})
        elapsed = {later - earlier for earlier, later in zip(ticks, ticks[1:])}
        self.assertEqual(elapsed, {6 * HOUR, 5 * HOUR})

    def test_fall_back_day_keeps_local_alignment_for_daily_ticks(self) -> None:
        start = datetime(2026, 10, 31, 12, 0, tzinfo=timezone.utc)
        end = datetime(2026, 11, 3, 12, 0, tzinfo=timezone.utc)

        ticks = aligned_ticks(start, end, DAY)

        self.assertEqual([(tick.month, tick.day, tick.hour) for tick in ticks], [(11, 1, 0), (11, 2, 0), (11, 3, 0)])
        self.assertEqual(ticks[1] - ticks[0], 25 * HOUR)

    def test_sub_day_labels_show_the_date_on_the_first_tick_of_each_day(self) -> None:
        start = datetime(2026, 10, 21, 20, 0, tzinfo=timezone.utc)
        end = datetime(2026, 10, 22, 8, 0, tzinfo=timezone.utc)
        ticks = aligned_ticks(start, end, 3 * HOUR)

        labels = x_tick_labels(ticks, 3 * HOUR)

        self.assertEqual(labels, ["18:00\nOct 21", "21:00", "00:00\nOct 22", "03:00"])

    def test_daily_labels_show_only_the_date(self) -> None:
        end = datetime(2026, 10, 21, 16, 30, tzinfo=timezone.utc)
        ticks = aligned_ticks(end - 3 * DAY, end, DAY)

        labels = x_tick_labels(ticks, DAY)

        self.assertEqual(labels, ["Oct 19", "Oct 20", "Oct 21"])


class ChartSegmentTests(NewYorkTimeTestCase):
    def test_points_further_apart_than_the_threshold_split_into_segments(self) -> None:
        minutes = (0, 1, 2, 300, 301)
        points = tuple(ChartPoint(NOW + timedelta(minutes=minute), 1.0) for minute in minutes)

        runs = split_at_gaps(points, timedelta(minutes=15))

        self.assertEqual([len(run) for run in runs], [3, 2])

    def test_points_exactly_at_the_threshold_stay_connected(self) -> None:
        points = (ChartPoint(NOW, 1.0), ChartPoint(NOW + timedelta(minutes=15), 1.0))

        self.assertEqual(len(split_at_gaps(points, timedelta(minutes=15))), 1)

    def test_outage_splits_the_chart_into_two_segments(self) -> None:
        first = [stored(NOW - timedelta(hours=10) + timedelta(minutes=minute), 295.0) for minute in range(0, 60, 5)]
        second = [stored(NOW - timedelta(hours=2) + timedelta(minutes=minute), 296.0) for minute in range(0, 60, 5)]

        chart = _build_chart(first + second, TEMPERATURE, NOW, STALE_AFTER)

        self.assertEqual(len(chart.segments), 2)
        self.assertFalse(any(segment.is_single_point for segment in chart.segments))

    def test_downsample_buckets_are_not_gaps(self) -> None:
        total = MAX_CHART_POINTS * 3
        history = [stored(NOW - timedelta(days=20) + timedelta(minutes=10 * index), 295.0) for index in range(total)]

        chart = _build_chart(history, TEMPERATURE, NOW, STALE_AFTER)

        self.assertEqual(len(chart.segments), 1)

    def test_isolated_reading_is_a_one_point_marker_segment(self) -> None:
        history = [
            stored(NOW - timedelta(hours=10), 295.0),
            stored(NOW - timedelta(hours=10) + timedelta(minutes=1), 295.5),
            stored(NOW - timedelta(hours=5), 296.0),
            stored(NOW - timedelta(hours=1), 295.0),
            stored(NOW - timedelta(hours=1) + timedelta(minutes=1), 295.2),
        ]

        chart = _build_chart(history, TEMPERATURE, NOW, STALE_AFTER)

        self.assertEqual([segment.is_single_point for segment in chart.segments], [False, True, False])
        marker = chart.segments[1]
        self.assertGreater(marker.x, PLOT_LEFT)
        self.assertLess(marker.x, PLOT_RIGHT_EDGE)

    def test_single_reading_is_one_marker_at_the_right_edge(self) -> None:
        chart = _build_chart([stored(NOW, 295.0)], TEMPERATURE, NOW, STALE_AFTER)

        self.assertEqual(len(chart.segments), 1)
        self.assertTrue(chart.segments[0].is_single_point)
        self.assertAlmostEqual(chart.segments[0].x, PLOT_RIGHT_EDGE, places=1)


class ChartGeometryTests(NewYorkTimeTestCase):
    def test_x_is_proportional_to_time_and_the_right_edge_is_now(self) -> None:
        history = [stored(NOW - timedelta(hours=10), 295.0), stored(NOW - timedelta(hours=5), 296.0)]

        chart = _build_chart(history, TEMPERATURE, NOW, timedelta(hours=10))

        coordinates = [pair.split(",") for pair in chart.segments[0].line.split()]
        self.assertAlmostEqual(float(coordinates[0][0]), PLOT_LEFT, places=1)
        self.assertAlmostEqual(float(coordinates[1][0]), PLOT_LEFT + 0.5 * (PLOT_RIGHT_EDGE - PLOT_LEFT), places=1)

    def test_window_starts_at_the_first_sample_inside_thirty_days(self) -> None:
        history = [stored(NOW - timedelta(days=2), 295.0), stored(NOW, 296.0)]

        chart = _build_chart(history, TEMPERATURE, NOW, timedelta(days=1))

        coordinates = [pair.split(",") for pair in chart.segments[0].line.split()]
        self.assertAlmostEqual(float(coordinates[0][0]), PLOT_LEFT, places=1)
        self.assertAlmostEqual(float(coordinates[-1][0]), PLOT_RIGHT_EDGE, places=1)

    def test_minimum_and_maximum_stay_raw(self) -> None:
        history = [stored(NOW - timedelta(hours=1), 293.15), stored(NOW, 296.15)]

        chart = _build_chart(history, TEMPERATURE, NOW, STALE_AFTER)

        self.assertAlmostEqual(chart.minimum or 0.0, 20.0)
        self.assertAlmostEqual(chart.maximum or 0.0, 23.0)
        self.assertLess(chart.y_ticks[0].value, 20.0 + 1e-9)
        self.assertGreater(chart.y_ticks[-1].value, 23.0 - 1e-9)

    def test_tick_positions_span_zero_to_one_and_percents_stay_in_the_box(self) -> None:
        history = [stored(NOW - timedelta(hours=30), 293.15), stored(NOW, 296.15)]

        chart = _build_chart(history, TEMPERATURE, NOW, STALE_AFTER)

        self.assertEqual(chart.y_ticks[0].position, 0.0)
        self.assertEqual(chart.y_ticks[-1].position, 1.0)
        self.assertGreater(chart.y_ticks[0].percent, chart.y_ticks[-1].percent)
        for tick in chart.x_ticks:
            self.assertTrue(0.0 <= tick.position <= 1.0)
            self.assertTrue(0.0 <= tick.percent <= 100.0)

    def test_every_other_x_label_hides_on_narrow_when_more_than_four(self) -> None:
        history = [stored(NOW - timedelta(hours=30), 293.15), stored(NOW, 296.15)]

        chart = _build_chart(history, TEMPERATURE, NOW, STALE_AFTER)

        self.assertGreater(len(chart.x_ticks), 4)
        self.assertEqual(
            [tick.hidden_on_narrow for tick in chart.x_ticks],
            [index % 2 == 1 for index in range(len(chart.x_ticks))],
        )

    def test_description_names_property_unit_span_low_high_and_latest(self) -> None:
        history = [stored(NOW - timedelta(days=2), 293.15), stored(NOW, 296.15)]

        chart = _build_chart(history, TEMPERATURE, NOW, STALE_AFTER)

        self.assertEqual(
            chart.description,
            "Temperature in °C, Oct 19 to Oct 21, 2 readings. Low 20.0, high 23.0, latest 23.0.",
        )

    def test_empty_history_gives_the_empty_chart(self) -> None:
        self.assertEqual(_build_chart([], TEMPERATURE, NOW, STALE_AFTER), TrendChart.empty())


if __name__ == "__main__":
    unittest.main()
