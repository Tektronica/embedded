import sys
import unittest
from types import ModuleType
from unittest.mock import Mock, PropertyMock, patch

from sensors.am2302 import AM2302Sensor
from sensors.analog import AnalogSoilMoisture, AnalogWindVane
from sensors.wind_rain import PulseAnemometer, TippingBucketRainGauge


def values_by_property(measurements: list[object]) -> dict[str, float]:
    return {item.property_id.rsplit(":", 1)[-1]: item.value for item in measurements}


class AM2302SensorTests(unittest.TestCase):
    def test_read_retries_transient_error_and_close_releases_device(self) -> None:
        device = Mock()
        board_stub = ModuleType("board")
        board_stub.D4 = object()
        dht_stub = ModuleType("adafruit_dht")
        dht_stub.DHT22 = Mock(return_value=device)

        with (
            patch.dict(sys.modules, {"board": board_stub, "adafruit_dht": dht_stub}),
            patch.object(
                type(device),
                "temperature",
                new_callable=PropertyMock,
                create=True,
                side_effect=[RuntimeError("transient read failure"), 22.5],
            ),
            patch.object(
                type(device),
                "humidity",
                new_callable=PropertyMock,
                create=True,
                return_value=48.0,
            ),
            patch("sensors.am2302.time.sleep") as sleep,
        ):
            sensor = AM2302Sensor(sensor_id="test-room", gpio_pin=4)
            readings = sensor.read()
            sensor.close()

        dht_stub.DHT22.assert_called_once_with(board_stub.D4, use_pulseio=False)
        sleep.assert_called_once_with(2)
        device.exit.assert_called_once_with()
        self.assertEqual(values_by_property(readings)["air-temperature"], 295.65)
        self.assertEqual(values_by_property(readings)["relative-humidity"], 0.48)


class AnalogSensorTests(unittest.TestCase):
    def test_wind_vane_emits_radians_and_voltage(self) -> None:
        adc = Mock()
        adc.channel.return_value.voltage = 1.75
        sensor = AnalogWindVane(adc, voltage_to_degrees=lambda voltage: 90.0)

        readings = sensor.read()
        values = values_by_property(readings)

        self.assertAlmostEqual(values["wind-direction"], 1.57079632679)
        self.assertEqual(values["adc-voltage"], 1.75)

    def test_soil_moisture_is_a_fraction_clamped_to_unit_interval(self) -> None:
        adc = Mock()
        adc.channel.return_value.voltage = 3.0
        sensor = AnalogSoilMoisture(adc, dry_voltage=2.8, wet_voltage=1.2)

        readings = sensor.read()

        self.assertEqual(values_by_property(readings)["soil-moisture"], 0.0)


class PulseSensorTests(unittest.TestCase):
    def test_anemometer_calculates_speed_and_windowed_gust(self) -> None:
        with (
            patch("sensors.wind_rain.Button") as button_factory,
            patch("sensors.wind_rain.time.monotonic", side_effect=[10.0, 12.0, 13.0]),
        ):
            sensor = PulseAnemometer(meters_per_second_per_hz=2.5)
            callback = button_factory.return_value.when_pressed
            for _ in range(4):
                callback()
            first = sensor.read()

            callback()
            second = sensor.read()
            sensor.close()

        first_values = values_by_property(first)
        second_values = values_by_property(second)
        self.assertEqual(first_values["wind-speed"], 5.0)
        self.assertEqual(first_values["wind-gust"], 5.0)
        self.assertEqual(second_values["wind-speed"], 2.5)
        self.assertEqual(second_values["wind-gust"], 5.0)
        button_factory.return_value.close.assert_called_once_with()

    def test_rain_rate_uses_new_tips_and_accumulation_uses_si_units(self) -> None:
        with (
            patch("sensors.wind_rain.Button") as button_factory,
            patch("sensors.wind_rain.time.monotonic", side_effect=[0.0, 3600.0, 7200.0]),
        ):
            sensor = TippingBucketRainGauge(mm_per_tip=0.25)
            callback = button_factory.return_value.when_pressed
            callback()
            callback()
            first = sensor.read()

            callback()
            second = sensor.read()
            sensor.close()

        first_values = values_by_property(first)
        second_values = values_by_property(second)
        self.assertAlmostEqual(first_values["rainfall-accumulation"], 0.0005)
        self.assertAlmostEqual(first_values["rainfall-rate"], 0.0000001388888889)
        self.assertAlmostEqual(second_values["rainfall-accumulation"], 0.00075)
        self.assertAlmostEqual(second_values["rainfall-rate"], 0.0000000694444444)
        button_factory.return_value.close.assert_called_once_with()


if __name__ == "__main__":
    unittest.main()