import unittest
from typing import Any

from weather_station.notifications.alarm_rules import AlarmRule, read_alarm_rules
from weather_station.notifications.errors import InvalidAlarmConfiguration

RULE = {"sensor": "sensor-am2302", "property": "air-temperature", "above": 35.0, "below": 0.0, "hysteresis": 1.0}


def read(*rules: Any) -> list[AlarmRule]:
    return read_alarm_rules({"alarms": list(rules)})


class AlarmRulesTests(unittest.TestCase):
    def test_reads_a_full_rule(self) -> None:
        self.assertEqual(read(RULE), [AlarmRule("sensor-am2302", "air-temperature", 35.0, 0.0, 1.0)])

    def test_missing_section_means_no_rules(self) -> None:
        self.assertEqual(read_alarm_rules({}), [])

    def test_one_threshold_and_default_hysteresis(self) -> None:
        rule = read({"sensor": "a", "property": "air-pressure", "below": 990})[0]

        self.assertEqual((rule.above, rule.below, rule.hysteresis), (None, 990.0, 0.0))

    def test_section_must_be_an_array(self) -> None:
        with self.assertRaisesRegex(InvalidAlarmConfiguration, "must be an array"):
            read_alarm_rules({"alarms": {}})

    def test_rule_must_be_an_object(self) -> None:
        with self.assertRaisesRegex(InvalidAlarmConfiguration, "must be an object"):
            read("x")

    def test_missing_both_thresholds_is_rejected(self) -> None:
        with self.assertRaisesRegex(InvalidAlarmConfiguration, "'above' or 'below'"):
            read({"sensor": "a", "property": "air-temperature", "hysteresis": 1})

    def test_bool_wrong_type_and_non_finite_numbers_are_rejected(self) -> None:
        for key in ("above", "below", "hysteresis"):
            for value in (True, "5", None, float("nan"), float("inf")):
                with self.subTest(key=key, value=value):
                    with self.assertRaisesRegex(InvalidAlarmConfiguration, "must be a number"):
                        read({"sensor": "a", "property": "air-temperature", "above": 1, key: value})

    def test_negative_hysteresis_is_rejected(self) -> None:
        with self.assertRaisesRegex(InvalidAlarmConfiguration, "negative"):
            read({**RULE, "hysteresis": -1})

    def test_unknown_key_is_rejected(self) -> None:
        with self.assertRaisesRegex(InvalidAlarmConfiguration, "unknown keys: abov"):
            read({**RULE, "abov": 3})

    def test_sensor_and_property_must_be_non_empty_strings(self) -> None:
        for key in ("sensor", "property"):
            for value in ("", 5):
                with self.subTest(key=key, value=value):
                    with self.assertRaisesRegex(InvalidAlarmConfiguration, key):
                        read({**RULE, key: value})

    def test_unknown_property_is_rejected(self) -> None:
        with self.assertRaisesRegex(InvalidAlarmConfiguration, "known property"):
            read({**RULE, "property": "temperature"})

    def test_duplicate_sensor_and_property_is_rejected(self) -> None:
        with self.assertRaisesRegex(InvalidAlarmConfiguration, "duplicate"):
            read(RULE, {**RULE, "above": 40})

    def test_same_property_on_another_sensor_is_allowed(self) -> None:
        self.assertEqual(len(read(RULE, {**RULE, "sensor": "sensor-bme280"})), 2)

    def test_band_must_exceed_twice_the_hysteresis(self) -> None:
        with self.assertRaisesRegex(InvalidAlarmConfiguration, "twice"):
            read({**RULE, "above": 2.0, "below": 0.0, "hysteresis": 1.0})
        self.assertEqual(len(read({**RULE, "above": 2.5, "below": 0.0, "hysteresis": 1.0})), 1)


if __name__ == "__main__":
    unittest.main()
