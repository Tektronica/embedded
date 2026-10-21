import unittest

from weather_station.shared.registry import SENSOR_DEFINITIONS


class SensorDefinitionTests(unittest.TestCase):
    def test_primary_property_is_one_of_the_sensor_properties(self) -> None:
        for definition in SENSOR_DEFINITIONS.values():
            with self.subTest(sensor_type=definition.key):
                self.assertIn(definition.primary_property, definition.properties)


if __name__ == "__main__":
    unittest.main()
