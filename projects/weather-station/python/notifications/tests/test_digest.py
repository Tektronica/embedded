import unittest
from datetime import datetime, timedelta, timezone

from weather_station.notifications.digest import DigestBuilder
from weather_station.shared.measurements import Measurement, StoredMeasurement
from weather_station.shared.registry import RELATIVE_HUMIDITY, TEMPERATURE
from weather_station.shared.units import CELSIUS, FAHRENHEIT

NOW = datetime(2026, 10, 20, 7, 0, tzinfo=timezone.utc)
LOCAL_NOW = datetime(2026, 10, 20, 7, 0, tzinfo=timezone(timedelta(hours=-5)))
KELVIN_AT_0_C = 273.15


def celsius(value: float) -> float:
    return value + KELVIN_AT_0_C


def stored(property_id: str, unit: str, value: float, observed_at: datetime) -> StoredMeasurement:
    return StoredMeasurement(
        "temperature_humidity",
        Measurement(
            sensor_id="sensor-am2302",
            observed_at=observed_at,
            property_id=property_id,
            value=value,
            unit=unit,
        ),
    )


def temperature(value_celsius: float, observed_at: datetime) -> StoredMeasurement:
    return stored(TEMPERATURE.property_id, TEMPERATURE.si_unit, celsius(value_celsius), observed_at)


def humidity(fraction: float, observed_at: datetime) -> StoredMeasurement:
    return stored(RELATIVE_HUMIDITY.property_id, RELATIVE_HUMIDITY.si_unit, fraction, observed_at)


class ReaderStub:
    def __init__(self, history: dict[str, list[StoredMeasurement]]) -> None:
        self._history = history

    def registered_sensors(self) -> list[tuple[str, str]]:
        return [("sensor-am2302", "temperature_humidity")] if self._history else []

    def sensor_type(self, sensor_id: str) -> str | None:
        return "temperature_humidity"

    def latest(self) -> list[StoredMeasurement]:
        return [items[-1] for items in self._history.values() if items]

    def latest_for_sensor(self, sensor_id: str) -> list[StoredMeasurement]:
        return self.latest()

    def series(
        self, sensor_id: str, property_id: str, since: datetime, limit: int = 3000
    ) -> list[StoredMeasurement]:
        items = [item for item in self._history[property_id] if item.measurement.observed_at >= since]
        return items[:limit]


class DigestBuilderTests(unittest.TestCase):
    def build_body(self, history: dict[str, list[StoredMeasurement]], now: datetime = NOW) -> str:
        return DigestBuilder(ReaderStub(history), CELSIUS).build(now).body

    def test_digest_lists_latest_value_with_observed_time_in_display_units(self) -> None:
        history = {
            TEMPERATURE.property_id: [temperature(20, NOW - timedelta(hours=1))],
            RELATIVE_HUMIDITY.property_id: [humidity(0.40, NOW - timedelta(hours=1))],
        }
        notification = DigestBuilder(ReaderStub(history), CELSIUS).build(NOW)
        body = notification.body

        self.assertEqual(notification.subject, "Weather station summary for 2026-10-20")
        self.assertIn("Temperature and humidity (sensor-am2302)", body)
        self.assertIn("Latest:          20.0 °C (2026-10-20 06:00 UTC)", body)
        self.assertIn("Latest:          40.0 % (2026-10-20 06:00 UTC)", body)

    def test_fahrenheit_default_converts_values_and_the_change_since_yesterday(self) -> None:
        history = {
            TEMPERATURE.property_id: [
                temperature(10, NOW - timedelta(hours=25)),
                temperature(20, NOW - timedelta(hours=1)),
            ],
            RELATIVE_HUMIDITY.property_id: [humidity(0.40, NOW - timedelta(hours=1))],
        }

        body = DigestBuilder(ReaderStub(history), FAHRENHEIT).build(NOW).body

        self.assertIn("Latest:          68.0 °F (2026-10-20 06:00 UTC)", body)
        self.assertIn("Latest:          40.0 % (2026-10-20 06:00 UTC)", body)
        self.assertIn("Since yesterday: +18.0 °F", body)

    def test_last_reading_time_is_shown_in_the_local_zone_of_now(self) -> None:
        body = self.build_body(
            {TEMPERATURE.property_id: [temperature(20, LOCAL_NOW - timedelta(hours=1))]},
            LOCAL_NOW,
        )

        self.assertIn("(2026-10-20 06:00 UTC-05:00)", body)

    def test_last_24_hours_reports_min_mean_and_max_and_ignores_older_readings(self) -> None:
        body = self.build_body({
            TEMPERATURE.property_id: [
                temperature(0, NOW - timedelta(hours=30)),
                temperature(10, NOW - timedelta(hours=20)),
                temperature(30, NOW - timedelta(hours=10)),
                temperature(20, NOW - timedelta(hours=1)),
            ],
        })

        self.assertIn("Last 24 h:       min 10.0, mean 20.0, max 30.0 °C", body)

    def test_change_since_yesterday_uses_reading_closest_to_24_hours_before_latest(self) -> None:
        latest_at = NOW - timedelta(hours=1)
        body = self.build_body({
            TEMPERATURE.property_id: [
                temperature(10, latest_at - timedelta(hours=24, minutes=20)),
                temperature(12, latest_at - timedelta(hours=24, minutes=5)),
                temperature(14, latest_at - timedelta(hours=23, minutes=40)),
                temperature(20, latest_at),
            ],
        })

        self.assertIn("Since yesterday: +8.0 °C", body)

    def test_change_since_yesterday_can_be_negative(self) -> None:
        latest_at = NOW - timedelta(hours=1)
        body = self.build_body({
            TEMPERATURE.property_id: [
                temperature(25, latest_at - timedelta(hours=24)),
                temperature(20, latest_at),
            ],
        })

        self.assertIn("Since yesterday: -5.0 °C", body)

    def test_change_since_yesterday_is_omitted_when_no_reading_is_within_tolerance(self) -> None:
        latest_at = NOW - timedelta(hours=1)
        body = self.build_body({
            TEMPERATURE.property_id: [
                temperature(10, latest_at - timedelta(hours=24, minutes=31)),
                temperature(14, latest_at - timedelta(hours=23, minutes=29)),
                temperature(20, latest_at),
            ],
        })

        self.assertNotIn("Since yesterday", body)

    def test_change_since_yesterday_accepts_a_reading_exactly_at_the_tolerance(self) -> None:
        latest_at = NOW - timedelta(hours=1)
        body = self.build_body({
            TEMPERATURE.property_id: [
                temperature(10, latest_at - timedelta(hours=24, minutes=30)),
                temperature(20, latest_at),
            ],
        })

        self.assertIn("Since yesterday: +10.0 °C", body)

    def test_daily_table_groups_by_local_day_across_the_midnight_boundary(self) -> None:
        body = self.build_body(
            {
                TEMPERATURE.property_id: [
                    temperature(10, datetime(2026, 10, 19, 4, 30, tzinfo=timezone.utc)),
                    temperature(18, datetime(2026, 10, 19, 15, 0, tzinfo=timezone.utc)),
                    temperature(5, datetime(2026, 10, 20, 4, 30, tzinfo=timezone.utc)),
                    temperature(7, datetime(2026, 10, 20, 5, 30, tzinfo=timezone.utc)),
                    temperature(12, datetime(2026, 10, 20, 11, 0, tzinfo=timezone.utc)),
                ],
            },
            LOCAL_NOW,
        )

        self.assertIn("2026-10-18  high   10.0  low   10.0 °C", body)
        self.assertIn("2026-10-19  high   18.0  low    5.0 °C", body)
        self.assertIn("2026-10-20  high   12.0  low    7.0 °C", body)

    def test_daily_table_is_oldest_first_skips_empty_days_and_covers_seven_local_days(self) -> None:
        def at_noon(day: int) -> datetime:
            return datetime(2026, 10, day, 12, 0, tzinfo=timezone.utc)

        body = self.build_body({
            TEMPERATURE.property_id: [
                temperature(1, at_noon(13)),
                temperature(2, at_noon(14)),
                temperature(3, at_noon(16)),
                temperature(4, NOW - timedelta(hours=1)),
            ],
        })

        rows = [line.split()[0] for line in body.splitlines() if line.strip().startswith("2026-")]
        self.assertEqual(rows, ["2026-10-14", "2026-10-16", "2026-10-20"])

    def test_series_is_requested_with_a_limit_that_covers_seven_days_of_minute_data(self) -> None:
        requested: list[int] = []

        class RecordingReader(ReaderStub):
            def series(
                self, sensor_id: str, property_id: str, since: datetime, limit: int = 3000
            ) -> list[StoredMeasurement]:
                requested.append(limit)
                return super().series(sensor_id, property_id, since, limit)

        DigestBuilder(RecordingReader({
            TEMPERATURE.property_id: [temperature(20, NOW - timedelta(hours=1))],
        }), CELSIUS).build(NOW)

        self.assertGreaterEqual(requested[0], 7 * 24 * 60)

    def test_property_with_only_a_stale_latest_reading_falls_back_to_the_latest_value(self) -> None:
        body = self.build_body({
            TEMPERATURE.property_id: [temperature(20, NOW - timedelta(days=10))],
        })

        self.assertIn("Last 24 h:       min 20.0, mean 20.0, max 20.0 °C", body)
        self.assertNotIn("Since yesterday", body)
        self.assertNotIn("Daily high and low", body)

    def test_sensor_without_readings_says_no_readings_yet(self) -> None:
        body = self.build_body({TEMPERATURE.property_id: []})

        self.assertIn("Temperature and humidity (sensor-am2302)\n  No readings yet.", body)

    def test_digest_with_no_sensors_says_so(self) -> None:
        notification = DigestBuilder(ReaderStub({}), CELSIUS).build(NOW)

        self.assertIn("No sensors have reported yet.", notification.body)


if __name__ == "__main__":
    unittest.main()
