import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import Any

from weather_station.sensors.config import SensorConfig, load_sensor_configs
from weather_station.sensors.detection import SensorDetector
from weather_station.sensors.errors import InvalidSensorConfiguration
from weather_station.sensors.factory import DEFAULT_BUILDERS, SensorFactory
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
            raise ValueError("No I2C device at address: 0x76")

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


class SensorFactoryTests(unittest.TestCase):
    def test_every_documented_model_has_a_builder(self) -> None:
        self.assertEqual(
            set(DEFAULT_BUILDERS),
            {"am2302", "bme280", "sgp30", "veml6075", "bh1750", "ds18b20", "as3935",
             "anemometer", "rain-gauge", "wind-vane", "soil-moisture"},
        )

    def test_unknown_option_is_rejected_before_touching_hardware(self) -> None:
        with self.assertRaisesRegex(InvalidSensorConfiguration, "unknown options: pin"):
            SensorFactory().build(SensorConfig("room", "am2302", {"pin": 4}))


class SensorConfigTests(unittest.TestCase):
    def test_loads_blocks_with_model_options(self) -> None:
        with TemporaryDirectory() as directory:
            path = Path(directory) / "sensors.toml"
            path.write_text(
                '[[sensor]]\nid = "room"\nmodel = "am2302"\ngpio = 4\n\n'
                '[[sensor]]\nid = "porch"\nmodel = "bme280"\ni2c_address = 0x77\n'
            )
            configs = load_sensor_configs(path)

        self.assertEqual(configs[0], SensorConfig("room", "am2302", {"gpio": 4}))
        self.assertEqual(configs[1].options, {"i2c_address": 0x77})

    def test_duplicate_ids_are_rejected(self) -> None:
        with TemporaryDirectory() as directory:
            path = Path(directory) / "sensors.toml"
            path.write_text('[[sensor]]\nid = "a"\nmodel = "am2302"\n\n[[sensor]]\nid = "a"\nmodel = "bme280"\n')
            with self.assertRaises(InvalidSensorConfiguration):
                load_sensor_configs(path)

    def test_repo_sensors_toml_loads(self) -> None:
        configs = load_sensor_configs(Path(__file__).resolve().parents[2] / "sensors.toml")

        self.assertEqual([config.model for config in configs], ["am2302"])


if __name__ == "__main__":
    unittest.main()
