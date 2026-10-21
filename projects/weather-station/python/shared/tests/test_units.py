import unittest

from weather_station.shared.registry import PRESSURE, RELATIVE_HUMIDITY, TEMPERATURE
from weather_station.shared.units import CELSIUS, FAHRENHEIT, UNIT_SYSTEMS, unit_system_for_code


class ConversionTests(unittest.TestCase):
    def test_kelvin_converts_to_fahrenheit_from_si(self) -> None:
        self.assertAlmostEqual(FAHRENHEIT.value(TEMPERATURE, 293.15), 68.0)
        self.assertAlmostEqual(FAHRENHEIT.value(TEMPERATURE, 273.15), 32.0)
        self.assertEqual(FAHRENHEIT.unit(TEMPERATURE), "°F")

    def test_kelvin_converts_to_celsius(self) -> None:
        self.assertEqual(CELSIUS.value(TEMPERATURE, 273.15), 0.0)
        self.assertEqual(CELSIUS.unit(TEMPERATURE), "°C")

    def test_properties_without_a_registered_conversion_keep_their_display_values(self) -> None:
        for system in (CELSIUS, FAHRENHEIT):
            with self.subTest(system=system.key):
                self.assertAlmostEqual(system.value(RELATIVE_HUMIDITY, 0.4), 40.0)
                self.assertEqual(system.unit(RELATIVE_HUMIDITY), "%")
                self.assertAlmostEqual(system.value(PRESSURE, 101300.0), 1013.0)
                self.assertEqual(system.unit(PRESSURE), "hPa")

    def test_default_display_value_round_trips_through_si(self) -> None:
        self.assertAlmostEqual(TEMPERATURE.si_value(35.0), 308.15)
        self.assertAlmostEqual(PRESSURE.si_value(1013.0), 101300.0)


class LookupTests(unittest.TestCase):
    def test_codes_map_to_systems(self) -> None:
        self.assertIs(unit_system_for_code("c"), CELSIUS)
        self.assertIs(unit_system_for_code("f"), FAHRENHEIT)

    def test_unknown_codes_map_to_none(self) -> None:
        for code in (None, "", "k", "F"):
            with self.subTest(code=code):
                self.assertIsNone(unit_system_for_code(code))

    def test_registry_is_keyed_by_config_value(self) -> None:
        self.assertEqual(set(UNIT_SYSTEMS), {"celsius", "fahrenheit"})


if __name__ == "__main__":
    unittest.main()
