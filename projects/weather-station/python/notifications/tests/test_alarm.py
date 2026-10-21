import smtplib
import unittest
from datetime import datetime, timedelta, timezone
from pathlib import Path
from tempfile import TemporaryDirectory

from weather_station.notifications.alarm import run_alarms
from weather_station.notifications.alarm_rules import AlarmRule
from weather_station.notifications.alarm_state import AlarmState, AlarmStateStore
from weather_station.notifications.contracts import Notification
from weather_station.notifications.settings import AlarmSettings
from weather_station.shared.measurements import Measurement, StoredMeasurement
from weather_station.shared.registry import TEMPERATURE
from weather_station.shared.units import CELSIUS, FAHRENHEIT

ZONE = timezone(timedelta(hours=-5), "EST")
NOW = datetime(2026, 10, 21, 14, 5, tzinfo=ZONE)
RULE = AlarmRule("room-am2302", "air-temperature", above=35.0, below=0.0, hysteresis=1.0)
STALE_AFTER = timedelta(minutes=5)


def temperature(celsius: float, observed_at: datetime, sensor_id: str = "room-am2302") -> StoredMeasurement:
    return StoredMeasurement(
        "temperature_humidity",
        Measurement(
            sensor_id=sensor_id,
            observed_at=observed_at,
            property_id=TEMPERATURE.property_id,
            value=celsius + 273.15,
            unit=TEMPERATURE.si_unit,
        ),
    )


class ReaderFake:
    def __init__(self, latest: list[StoredMeasurement]) -> None:
        self._latest = latest
        self.reads = 0

    def registered_sensors(self) -> list[tuple[str, str]]:
        return []

    def sensor_type(self, sensor_id: str) -> str | None:
        return None

    def latest(self) -> list[StoredMeasurement]:
        self.reads += 1
        return self._latest

    def latest_for_sensor(self, sensor_id: str) -> list[StoredMeasurement]:
        return []

    def series(self, sensor_id: str, property_id: str, since: datetime, limit: int = 3000) -> list[StoredMeasurement]:
        return []


class NotifierFake:
    def __init__(self, failure: Exception | None = None) -> None:
        self.sent: list[Notification] = []
        self.created = 0
        self._failure = failure

    def create(self) -> "NotifierFake":
        self.created += 1
        return self

    def send(self, notification: Notification) -> None:
        if self._failure:
            raise self._failure
        self.sent.append(notification)


class RunAlarmsTests(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = TemporaryDirectory()
        self.store = AlarmStateStore(Path(self.directory.name) / "alarm-state.json")
        self.settings = AlarmSettings(
            database_path=Path(self.directory.name) / "weather.sqlite3",
            state_path=self.store._path,
            stale_after=STALE_AFTER,
            rules=[RULE],
            unit_system=CELSIUS,
        )

    def tearDown(self) -> None:
        self.directory.cleanup()

    def test_no_events_never_creates_the_notifier_and_saves_state(self) -> None:
        notifier = NotifierFake()

        succeeded = run_alarms(self.settings, ReaderFake([temperature(20.0, NOW)]), self.store, notifier.create, NOW)

        self.assertTrue(succeeded)
        self.assertEqual(notifier.created, 0)
        self.assertEqual(self.store.load(), AlarmState.empty())

    def test_events_send_one_email_and_save_state(self) -> None:
        notifier = NotifierFake()
        reader = ReaderFake([temperature(36.2, NOW - timedelta(minutes=2))])

        succeeded = run_alarms(self.settings, reader, self.store, notifier.create, NOW)

        self.assertTrue(succeeded)
        self.assertEqual(len(notifier.sent), 1)
        self.assertEqual(
            notifier.sent[0].body, "room-am2302 temperature above 35.0 °C: now 36.2 °C (2026-10-21 14:03 EST)\n"
        )
        self.assertEqual(self.store.load().thresholds, {RULE.key: frozenset({"above"})})

    def test_fahrenheit_default_prints_fahrenheit_and_still_compares_in_celsius(self) -> None:
        self.settings = AlarmSettings(
            self.settings.database_path, self.settings.state_path, STALE_AFTER, [RULE], FAHRENHEIT
        )
        notifier = NotifierFake()

        run_alarms(self.settings, ReaderFake([temperature(30.0, NOW)]), self.store, notifier.create, NOW)
        run_alarms(self.settings, ReaderFake([temperature(36.2, NOW)]), self.store, notifier.create, NOW)

        self.assertEqual(
            [item.body for item in notifier.sent],
            ["room-am2302 temperature above 95.0 °F: now 97.2 °F (2026-10-21 14:05 EST)\n"],
        )

    def test_second_run_with_the_same_reading_sends_nothing(self) -> None:
        notifier = NotifierFake()
        reader = ReaderFake([temperature(36.2, NOW)])

        run_alarms(self.settings, reader, self.store, notifier.create, NOW)
        run_alarms(self.settings, reader, self.store, notifier.create, NOW)

        self.assertEqual(len(notifier.sent), 1)

    def test_many_transitions_share_one_email(self) -> None:
        notifier = NotifierFake()
        other = AlarmRule("porch", "air-temperature", above=30.0, below=None, hysteresis=0.0)
        self.settings = AlarmSettings(
            self.settings.database_path, self.settings.state_path, STALE_AFTER, [RULE, other], CELSIUS
        )
        reader = ReaderFake([temperature(36.0, NOW), temperature(31.0, NOW, "porch")])

        run_alarms(self.settings, reader, self.store, notifier.create, NOW)

        self.assertEqual(len(notifier.sent), 1)
        self.assertEqual(notifier.sent[0].subject, "Weather station alarm: 2 events")

    def test_send_failure_keeps_state_and_returns_false(self) -> None:
        notifier = NotifierFake(smtplib.SMTPAuthenticationError(535, b"bad credentials"))
        previous = AlarmState({RULE.key: frozenset({"below"})}, frozenset())
        self.store.save(previous)

        with self.assertLogs(level="ERROR"):
            succeeded = run_alarms(
                self.settings, ReaderFake([temperature(36.2, NOW)]), self.store, notifier.create, NOW
            )

        self.assertFalse(succeeded)
        self.assertEqual(self.store.load(), previous)

    def test_no_rules_skips_the_database(self) -> None:
        self.settings = AlarmSettings(self.settings.database_path, self.settings.state_path, STALE_AFTER, [], CELSIUS)
        reader = ReaderFake([])

        self.assertTrue(run_alarms(self.settings, reader, self.store, NotifierFake().create, NOW))
        self.assertEqual(reader.reads, 0)

    def test_sensor_missing_from_the_database_is_reported_stale(self) -> None:
        notifier = NotifierFake()

        run_alarms(self.settings, ReaderFake([]), self.store, notifier.create, NOW)

        self.assertEqual(notifier.sent[0].body, "room-am2302 stale: no readings yet\n")


if __name__ == "__main__":
    unittest.main()
