import smtplib
import sqlite3
import unittest
from unittest import mock
from datetime import datetime, timedelta, timezone
from pathlib import Path
from tempfile import TemporaryDirectory
from uuid import uuid4

from weather_station.notifications import boot
from weather_station.notifications.boot import (
    format_offline_gap,
    last_reading_before,
    read_uptime,
    run_boot,
)
from weather_station.notifications.contracts import Notification
from weather_station.shared.db import SCHEMA_VERSION, schema_sql
from weather_station.shared.db.sqlite_reader import SQLiteMeasurementReader
from weather_station.shared.measurements import Measurement, StoredMeasurement
from weather_station.shared.registry import TEMPERATURE

ZONE = timezone(timedelta(hours=-5), "EST")
NOW = datetime(2026, 10, 21, 14, 5, tzinfo=ZONE)
UPTIME = timedelta(minutes=5)
STARTED_AT = NOW - UPTIME


def temperature(observed_at: datetime, sensor_id: str = "room-am2302") -> StoredMeasurement:
    return StoredMeasurement(
        "temperature_humidity",
        Measurement(
            sensor_id=sensor_id,
            observed_at=observed_at,
            property_id=TEMPERATURE.property_id,
            value=293.15,
            unit=TEMPERATURE.si_unit,
        ),
    )


class ReaderFake:
    def __init__(self, latest: list[StoredMeasurement]) -> None:
        self._latest = latest

    def registered_sensors(self) -> list[tuple[str, str]]:
        return []

    def sensor_type(self, sensor_id: str) -> str | None:
        return None

    def latest(self) -> list[StoredMeasurement]:
        return self._latest

    def latest_before(self, cutoff: datetime) -> list[StoredMeasurement]:
        return [item for item in self._latest if item.measurement.observed_at < cutoff]

    def latest_for_sensor(self, sensor_id: str) -> list[StoredMeasurement]:
        return []

    def series(self, sensor_id: str, property_id: str, since: datetime, limit: int = 3000) -> list[StoredMeasurement]:
        return []


class NotifierFake:
    def __init__(self, failure: Exception | None = None) -> None:
        self.sent: list[Notification] = []
        self._failure = failure

    def create(self) -> "NotifierFake":
        return self

    def send(self, notification: Notification) -> None:
        if self._failure:
            raise self._failure
        self.sent.append(notification)


class ReadUptimeTests(unittest.TestCase):
    def test_reads_the_first_field_of_the_uptime_file(self) -> None:
        with TemporaryDirectory() as directory:
            path = Path(directory) / "uptime"
            path.write_text("12345.67 54321.00\n")

            self.assertEqual(read_uptime(path), timedelta(seconds=12345.67))


class FormatOfflineGapTests(unittest.TestCase):
    def test_formats_minutes_hours_and_days(self) -> None:
        cases = {
            timedelta(seconds=20): "less than 1 min",
            timedelta(minutes=12): "12 min",
            timedelta(hours=3, minutes=12): "3 h 12 min",
            timedelta(days=1, hours=4): "1 day 4 h",
            timedelta(days=2, hours=1, minutes=40): "2 days 1 h",
        }
        for gap, expected in cases.items():
            with self.subTest(gap=gap):
                self.assertEqual(format_offline_gap(gap), expected)


class LastReadingBeforeTests(unittest.TestCase):
    def test_ignores_readings_after_the_start(self) -> None:
        before = temperature(STARTED_AT - timedelta(hours=1))
        reader = ReaderFake([before, temperature(STARTED_AT + timedelta(minutes=1), "porch")])

        self.assertEqual(last_reading_before(reader, STARTED_AT), before.measurement)

    def test_picks_the_newest_across_sensors(self) -> None:
        newest = temperature(STARTED_AT - timedelta(minutes=10), "porch")
        reader = ReaderFake([temperature(STARTED_AT - timedelta(hours=2)), newest])

        self.assertEqual(last_reading_before(reader, STARTED_AT), newest.measurement)

    def test_no_readings_returns_none(self) -> None:
        self.assertIsNone(last_reading_before(ReaderFake([]), STARTED_AT))

    def test_missing_database_returns_none(self) -> None:
        with TemporaryDirectory() as directory:
            reader = SQLiteMeasurementReader(Path(directory) / "missing.sqlite3")

            self.assertIsNone(last_reading_before(reader, STARTED_AT))

    def test_reading_after_the_start_does_not_hide_the_earlier_one(self) -> None:
        earlier = STARTED_AT - timedelta(hours=1)
        with TemporaryDirectory() as directory:
            path = Path(directory) / "weather.sqlite3"
            with sqlite3.connect(path) as connection:
                connection.executescript(schema_sql())
                connection.execute("INSERT INTO schema_version (version) VALUES (?)", (SCHEMA_VERSION,))
                connection.execute("INSERT INTO sensors VALUES ('room-am2302', 'temperature_humidity')")
                for observed_at in (earlier, STARTED_AT + timedelta(minutes=1)):
                    utc_time = observed_at.astimezone(timezone.utc).isoformat()
                    connection.execute(
                        "INSERT INTO measurements VALUES (?, ?, ?, 'room-am2302', ?, 293.15, ?, 'valid', NULL)",
                        (str(uuid4()), utc_time, utc_time, TEMPERATURE.property_id, TEMPERATURE.si_unit),
                    )

            last = last_reading_before(SQLiteMeasurementReader(path), STARTED_AT)

        self.assertIsNotNone(last)
        self.assertEqual(last.observed_at, earlier)


class RunBootTests(unittest.TestCase):
    def test_sends_one_email_with_the_start_and_the_gap(self) -> None:
        notifier = NotifierFake()
        reader = ReaderFake([temperature(STARTED_AT - timedelta(hours=3, minutes=12))])

        succeeded = run_boot(reader, notifier.create, NOW, lambda: UPTIME)

        self.assertTrue(succeeded)
        self.assertEqual(len(notifier.sent), 1)
        self.assertEqual(notifier.sent[0].subject, "Weather station started")
        self.assertEqual(
            notifier.sent[0].body,
            "The weather station started at 2026-10-21 14:00 EST.\n"
            "Last reading before the start: 2026-10-21 10:48 EST (room-am2302).\n"
            "Offline for about 3 h 12 min.\n",
        )

    def test_no_earlier_reading_says_so(self) -> None:
        notifier = NotifierFake()

        run_boot(ReaderFake([]), notifier.create, NOW, lambda: UPTIME)

        self.assertEqual(
            notifier.sent[0].body,
            "The weather station started at 2026-10-21 14:00 EST.\nNo earlier reading was found.\n",
        )

    def test_send_failure_returns_false(self) -> None:
        notifier = NotifierFake(smtplib.SMTPAuthenticationError(535, b"bad credentials"))

        with self.assertLogs(level="ERROR"):
            succeeded = run_boot(ReaderFake([]), notifier.create, NOW, lambda: UPTIME)

        self.assertFalse(succeeded)


class MainTests(unittest.TestCase):
    def test_send_failure_exits_non_zero(self) -> None:
        with (
            mock.patch.object(boot.NotificationSettings, "from_config"),
            mock.patch.object(boot, "SQLiteMeasurementReader"),
            mock.patch.object(boot, "run_boot", return_value=False),
            self.assertRaises(SystemExit) as raised,
        ):
            boot.main()

        self.assertEqual(raised.exception.code, 1)


if __name__ == "__main__":
    unittest.main()
