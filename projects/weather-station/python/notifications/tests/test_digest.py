import unittest
from datetime import datetime, timedelta, timezone

from weather_station.notifications.digest import DigestBuilder
from weather_station.shared.measurements import Measurement, StoredMeasurement
from weather_station.shared.registry import RELATIVE_HUMIDITY, TEMPERATURE

NOW = datetime(2026, 10, 20, 7, 0, tzinfo=timezone.utc)


def stored(property_id: str, unit: str, value: float, hours_ago: float) -> StoredMeasurement:
    return StoredMeasurement(
        "temperature_humidity",
        Measurement(
            sensor_id="room-am2302",
            observed_at=NOW - timedelta(hours=hours_ago),
            property_id=property_id,
            value=value,
            unit=unit,
        ),
    )


class ReaderStub:
    def __init__(self, history: dict[str, list[StoredMeasurement]]) -> None:
        self._history = history

    def registered_sensors(self) -> list[tuple[str, str]]:
        return [("room-am2302", "temperature_humidity")] if self._history else []

    def sensor_type(self, sensor_id: str) -> str | None:
        return "temperature_humidity"

    def latest(self) -> list[StoredMeasurement]:
        return [items[-1] for items in self._history.values()]

    def latest_for_sensor(self, sensor_id: str) -> list[StoredMeasurement]:
        return self.latest()

    def series(
        self, sensor_id: str, property_id: str, since: datetime, limit: int = 3000
    ) -> list[StoredMeasurement]:
        return [item for item in self._history[property_id] if item.measurement.observed_at >= since]


class DigestBuilderTests(unittest.TestCase):
    def test_digest_lists_latest_value_and_24_hour_range_in_display_units(self) -> None:
        reader = ReaderStub({
            TEMPERATURE.property_id: [
                stored(TEMPERATURE.property_id, TEMPERATURE.si_unit, 280.15, 30),
                stored(TEMPERATURE.property_id, TEMPERATURE.si_unit, 288.15, 20),
                stored(TEMPERATURE.property_id, TEMPERATURE.si_unit, 298.15, 10),
                stored(TEMPERATURE.property_id, TEMPERATURE.si_unit, 293.15, 1),
            ],
            RELATIVE_HUMIDITY.property_id: [
                stored(RELATIVE_HUMIDITY.property_id, RELATIVE_HUMIDITY.si_unit, 0.40, 1),
            ],
        })

        notification = DigestBuilder(reader).build(NOW)

        self.assertEqual(notification.subject, "Weather station summary for 2026-10-20")
        self.assertIn("Temperature and humidity (room-am2302)", notification.body)
        self.assertIn("Temperature: 20.0 °C (24 h range 15.0 to 25.0 °C", notification.body)
        self.assertIn("Relative humidity: 40.0 %", notification.body)

    def test_digest_with_no_sensors_says_so(self) -> None:
        notification = DigestBuilder(ReaderStub({})).build(NOW)

        self.assertIn("No sensors have reported yet.", notification.body)


if __name__ == "__main__":
    unittest.main()
