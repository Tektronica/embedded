import json
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import Any

from weather_station.sensors.config import SensorConfig, parse_sensor_configs
from weather_station.sensors.detection import SensorDetector
from weather_station.sensors.errors import InvalidCollectorConfiguration, InvalidSensorConfiguration
from weather_station.sensors.factory import DEFAULT_BUILDERS, SensorFactory
from weather_station.sensors.settings import CollectorSettings
from weather_station.shared.measurements import Measurement


class StubSensor:
    sensor_type = "temperature_humidity"

    def __init__(self, sensor_id: str, readable: bool) -> None:
        self.sensor_id = sensor_id
        self.readable = readable
        self.closed = False

    def read(self) -> list[Measurement]:
        if not self.readable:
            raise RuntimeError("no response on GPIO 4")
        return []

    def close(self) -> None:
        self.closed = True


class SensorDetectorTests(unittest.TestCase):
    def setUp(self) -> None:
        self.built: list[StubSensor] = []

        def present(sensor_id: str, options: dict[str, Any], factory: SensorFactory) -> StubSensor:
            sensor = StubSensor(sensor_id, readable=True)
            self.built.append(sensor)
            return sensor

        def unplugged(sensor_id: str, options: dict[str, Any], factory: SensorFactory) -> StubSensor:
            sensor = StubSensor(sensor_id, readable=False)
            self.built.append(sensor)
            return sensor

        def missing_bus(sensor_id: str, options: dict[str, Any], factory: SensorFactory) -> StubSensor:
            raise OSError("No I2C device at address: 0x76")

        self.factory = SensorFactory({"present": present, "unplugged": unplugged, "missing-bus": missing_bus})

    def test_keeps_only_sensors_that_answer(self) -> None:
        configs = [
            SensorConfig("a", "present"),
            SensorConfig("b", "unplugged"),
            SensorConfig("c", "missing-bus"),
        ]

        detected = SensorDetector(self.factory).detect(configs)

        self.assertEqual([item.sensor.sensor_id for item in detected], ["a"])
        self.assertTrue(self.built[1].closed)

    def test_unknown_model_is_a_configuration_error(self) -> None:
        with self.assertRaises(InvalidSensorConfiguration):
            SensorDetector(self.factory).detect([SensorConfig("a", "made-up")])

    def test_invalid_option_value_is_not_treated_as_missing_hardware(self) -> None:
        with self.assertRaises(InvalidSensorConfiguration):
            SensorDetector(SensorFactory()).detect(
                [SensorConfig("sensor-am2302", "am2302", {"gpio": "four"})]
            )


class SensorFactoryTests(unittest.TestCase):
    def test_every_documented_model_has_a_builder(self) -> None:
        self.assertEqual(
            set(DEFAULT_BUILDERS),
            {"am2302", "bme280", "sgp30", "veml6075", "bh1750", "ds18b20", "as3935",
             "anemometer", "rain-gauge", "wind-vane", "soil-moisture"},
        )

    def test_unknown_option_is_rejected_before_touching_hardware(self) -> None:
        with self.assertRaisesRegex(InvalidSensorConfiguration, "unknown options: pin"):
            SensorFactory().build(SensorConfig("sensor-am2302", "am2302", {"pin": 4}))

    def test_wrong_option_type_and_range_are_configuration_errors(self) -> None:
        for options in ({"gpio": "four"}, {"gpio": 40}):
            with self.subTest(options=options):
                with self.assertRaises(InvalidSensorConfiguration):
                    SensorFactory().build(SensorConfig("sensor-am2302", "am2302", options))

    def test_nonfinite_calibration_is_a_configuration_error(self) -> None:
        with self.assertRaisesRegex(InvalidSensorConfiguration, "must be finite"):
            SensorFactory().build(
                SensorConfig("wind", "anemometer", {"meters_per_second_per_hz": float("nan")})
            )


class SensorConfigTests(unittest.TestCase):
    def test_parses_devices_with_model_options(self) -> None:
        configs = parse_sensor_configs(
            [
                {"id": "sensor-am2302", "model": "am2302", "gpio": 4},
                {"id": "sensor-bme280", "model": "bme280", "i2c_address": 119},
            ]
        )

        self.assertEqual(configs[0], SensorConfig("sensor-am2302", "am2302", {"gpio": 4}))
        self.assertEqual(configs[1].options, {"i2c_address": 0x77})

    def test_duplicate_ids_are_rejected(self) -> None:
        with self.assertRaisesRegex(InvalidSensorConfiguration, "unique"):
            parse_sensor_configs(
                [{"id": "a", "model": "am2302"}, {"id": "a", "model": "bme280"}]
            )

    def test_devices_must_be_an_array(self) -> None:
        with self.assertRaisesRegex(InvalidSensorConfiguration, "array of objects"):
            parse_sensor_configs("not an array")

    def test_each_device_must_be_an_object(self) -> None:
        with self.assertRaisesRegex(InvalidSensorConfiguration, "JSON object"):
            parse_sensor_configs(["am2302"])

    def test_id_must_be_a_non_empty_trimmed_string(self) -> None:
        for sensor_id in ("", " sensor-am2302", 7, None):
            with self.subTest(sensor_id=sensor_id):
                with self.assertRaisesRegex(InvalidSensorConfiguration, "non-empty id"):
                    parse_sensor_configs([{"id": sensor_id, "model": "am2302"}])

    def test_model_must_be_a_string(self) -> None:
        with self.assertRaisesRegex(InvalidSensorConfiguration, "needs a model"):
            parse_sensor_configs([{"id": "sensor-am2302", "model": 4}])

    def test_empty_devices_list_is_valid(self) -> None:
        self.assertEqual(parse_sensor_configs([]), [])


class CollectorSettingsTests(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = TemporaryDirectory()
        self.path = Path(self.directory.name) / "config.json"

    def tearDown(self) -> None:
        self.directory.cleanup()

    def _load(self, document: Any) -> CollectorSettings:
        self.path.write_text(json.dumps(document))
        return CollectorSettings.from_config(self.path)

    def test_reads_every_key(self) -> None:
        settings = self._load(
            {
                "storage": {"database_path": "data/weather.sqlite3"},
                "sensors": {
                    "interval_seconds": 30,
                    "retention_days": 7,
                    "devices": [{"id": "sensor-am2302", "model": "am2302", "gpio": 4}],
                },
            }
        )

        self.assertEqual(settings.database_path, Path("data/weather.sqlite3"))
        self.assertEqual(settings.interval_seconds, 30)
        self.assertEqual(settings.retention_days, 7)
        self.assertEqual(settings.sensors, [SensorConfig("sensor-am2302", "am2302", {"gpio": 4})])

    def test_absent_keys_use_defaults(self) -> None:
        settings = self._load({})

        self.assertEqual(settings.database_path, Path("instance/weather.sqlite3"))
        self.assertEqual(settings.interval_seconds, 60)
        self.assertEqual(settings.retention_days, 30)
        self.assertEqual(settings.sensors, [])

    def test_section_must_be_an_object(self) -> None:
        with self.assertRaisesRegex(InvalidCollectorConfiguration, "must be an object"):
            self._load({"sensors": []})

    def test_unknown_key_is_rejected(self) -> None:
        with self.assertRaisesRegex(InvalidCollectorConfiguration, "intervall_seconds"):
            self._load({"sensors": {"intervall_seconds": 60}})

    def test_interval_and_retention_reject_wrong_types_and_bool(self) -> None:
        for key in ("interval_seconds", "retention_days"):
            for value in ("60", 1.5, True):
                with self.subTest(key=key, value=value):
                    with self.assertRaisesRegex(InvalidCollectorConfiguration, "integer"):
                        self._load({"sensors": {key: value}})

    def test_interval_and_retention_must_be_positive(self) -> None:
        for key in ("interval_seconds", "retention_days"):
            for value in (0, -1):
                with self.subTest(key=key, value=value):
                    with self.assertRaisesRegex(InvalidCollectorConfiguration, "positive"):
                        self._load({"sensors": {key: value}})

    def test_example_config_loads(self) -> None:
        example = Path(__file__).resolve().parents[2] / "config.example.json"
        settings = CollectorSettings.from_config(example)

        self.assertEqual([config.model for config in settings.sensors], ["am2302"])


if __name__ == "__main__":
    unittest.main()
