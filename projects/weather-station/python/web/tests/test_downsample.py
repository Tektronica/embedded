import unittest
from datetime import datetime, timedelta, timezone

from weather_station.web.assemblers.dashboard import ChartPoint, downsample

START = datetime(2026, 10, 1, tzinfo=timezone.utc)


def point(minutes: int, value: float) -> ChartPoint:
    return ChartPoint(timestamp=START + timedelta(minutes=minutes), value=value)


class DownsampleTests(unittest.TestCase):
    def test_series_at_cap_passes_through(self) -> None:
        samples = [point(index, float(index)) for index in range(4)]

        self.assertEqual(downsample(samples, 4), tuple(samples))

    def test_series_below_cap_passes_through(self) -> None:
        samples = [point(0, 1.0), point(1, 2.0)]

        self.assertEqual(downsample(samples, 4), tuple(samples))

    def test_series_above_cap_averages_equal_buckets(self) -> None:
        samples = [point(index, float(index)) for index in range(9)]

        result = downsample(samples, 4)

        self.assertEqual(len(result), 4)
        self.assertEqual([item.value for item in result], [0.5, 2.5, 4.5, 7.0])
        self.assertEqual(
            [item.timestamp for item in result],
            [START + timedelta(minutes=minutes) for minutes in (1, 3, 5, 7)],
        )

    def test_empty_buckets_produce_no_point(self) -> None:
        samples = [point(0, 10.0), point(1, 20.0), point(2, 30.0), point(40, 50.0), point(41, 70.0)]

        result = downsample(samples, 4)

        self.assertEqual([item.value for item in result], [20.0, 60.0])
        self.assertEqual(
            [item.timestamp for item in result],
            [START + timedelta(minutes=5.125), START + timedelta(minutes=35.875)],
        )

    def test_identical_timestamps_pass_through(self) -> None:
        samples = [point(0, 1.0), point(0, 2.0), point(0, 3.0)]

        self.assertEqual(downsample(samples, 2), tuple(samples))


if __name__ == "__main__":
    unittest.main()
