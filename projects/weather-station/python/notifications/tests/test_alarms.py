import unittest
from datetime import datetime, timedelta, timezone

from weather_station.notifications.alarm_rules import AlarmRule
from weather_station.notifications.alarm_state import AlarmState
from weather_station.notifications.alarms import (
    AlarmReading,
    FreshnessEvent,
    FreshnessEventKind,
    SensorSnapshot,
    ThresholdEvent,
    ThresholdEventKind,
    evaluate_alarms,
    format_notification,
)

ZONE = timezone(timedelta(hours=-5), "EST")
NOW = datetime(2026, 10, 21, 14, 5, tzinfo=ZONE)
STALE_AFTER = timedelta(minutes=5)
RULE = AlarmRule("room", "air-temperature", above=35.0, below=0.0, hysteresis=1.0)
ABOVE_ONLY = AlarmRule("room", "air-temperature", above=35.0, below=None, hysteresis=1.0)
BELOW_ONLY = AlarmRule("room", "air-temperature", above=None, below=0.0, hysteresis=1.0)


def snapshot(value: float, age: timedelta = timedelta(0)) -> dict[str, SensorSnapshot]:
    observed_at = NOW - age
    reading = AlarmReading("Temperature", "°C", value, observed_at)
    return {"room": SensorSnapshot(observed_at, {"air-temperature": reading})}


def state(*sides: str, stale: bool = False) -> AlarmState:
    thresholds = {RULE.key: frozenset(sides)} if sides else {}
    return AlarmState(thresholds, frozenset({"room"}) if stale else frozenset())


def kinds(events: list[ThresholdEvent | FreshnessEvent]) -> list[str]:
    return [event.kind.value for event in events]


class ThresholdTests(unittest.TestCase):
    def evaluate(self, rule: AlarmRule, value: float, previous: AlarmState) -> tuple[AlarmState, list[str]]:
        new_state, events = evaluate_alarms([rule], snapshot(value), previous, NOW, STALE_AFTER)
        return new_state, kinds(events)

    def test_above_triggers_when_value_passes_the_limit(self) -> None:
        new_state, events = self.evaluate(ABOVE_ONLY, 35.1, state())

        self.assertEqual(events, ["above"])
        self.assertEqual(new_state, state("above"))

    def test_value_equal_to_the_limit_does_not_trigger(self) -> None:
        self.assertEqual(self.evaluate(ABOVE_ONLY, 35.0, state()), (state(), []))

    def test_above_holds_inside_the_band(self) -> None:
        self.assertEqual(self.evaluate(ABOVE_ONLY, 34.5, state("above")), (state("above"), []))
        self.assertEqual(self.evaluate(ABOVE_ONLY, 34.0, state("above")), (state("above"), []))

    def test_above_clears_below_the_band(self) -> None:
        self.assertEqual(self.evaluate(ABOVE_ONLY, 33.9, state("above")), (state(), ["back below"]))

    def test_below_triggers_holds_and_clears(self) -> None:
        self.assertEqual(self.evaluate(BELOW_ONLY, -0.1, state()), (state("below"), ["below"]))
        self.assertEqual(self.evaluate(BELOW_ONLY, 0.5, state("below")), (state("below"), []))
        self.assertEqual(self.evaluate(BELOW_ONLY, 1.0, state("below")), (state("below"), []))
        self.assertEqual(self.evaluate(BELOW_ONLY, 1.1, state("below")), (state(), ["back above"]))

    def test_inside_the_band_with_no_prior_state_stays_quiet(self) -> None:
        self.assertEqual(self.evaluate(RULE, 20.0, state()), (state(), []))

    def test_sequence_around_the_threshold_alerts_once_per_crossing(self) -> None:
        current = state()
        sent: list[str] = []
        for value in (34.9, 35.1, 34.9, 35.2, 34.5, 33.9, 34.9, 35.1):
            current, events = self.evaluate(ABOVE_ONLY, value, current)
            sent.extend(events)

        self.assertEqual(sent, ["above", "back below", "above"])

    def test_first_run_past_a_threshold_triggers(self) -> None:
        self.assertEqual(self.evaluate(RULE, 40.0, AlarmState.empty()), (state("above"), ["above"]))
        self.assertEqual(self.evaluate(RULE, -5.0, AlarmState.empty()), (state("below"), ["below"]))

    def test_event_carries_the_limit_and_reading(self) -> None:
        _, events = evaluate_alarms([RULE], snapshot(33.0), state("above"), NOW, STALE_AFTER)

        event = events[0]
        assert isinstance(event, ThresholdEvent)
        self.assertEqual(event.kind, ThresholdEventKind.BACK_BELOW)
        self.assertEqual(event.limit, 34.0)
        self.assertEqual(event.reading.value, 33.0)


class StaleTests(unittest.TestCase):
    def test_old_reading_triggers_stale(self) -> None:
        old = snapshot(20.0, age=timedelta(minutes=6))

        new_state, events = evaluate_alarms([RULE], old, state(), NOW, STALE_AFTER)

        self.assertEqual(kinds(events), ["stale"])
        self.assertEqual(new_state, state(stale=True))

    def test_reading_exactly_at_the_limit_is_fresh(self) -> None:
        _, events = evaluate_alarms([RULE], snapshot(20.0, age=STALE_AFTER), state(), NOW, STALE_AFTER)

        self.assertEqual(events, [])

    def test_stale_alerts_once_while_it_lasts(self) -> None:
        old = snapshot(20.0, age=timedelta(minutes=30))

        new_state, events = evaluate_alarms([RULE], old, state(stale=True), NOW, STALE_AFTER)

        self.assertEqual((new_state, events), (state(stale=True), []))

    def test_fresh_reading_clears_stale(self) -> None:
        new_state, events = evaluate_alarms([RULE], snapshot(20.0), state(stale=True), NOW, STALE_AFTER)

        self.assertEqual(kinds(events), ["reporting"])
        self.assertEqual(new_state, state())

    def test_thresholds_hold_while_stale(self) -> None:
        old = snapshot(10.0, age=timedelta(minutes=30))

        new_state, events = evaluate_alarms([RULE], old, state("above", stale=True), NOW, STALE_AFTER)

        self.assertEqual((new_state, events), (state("above", stale=True), []))

    def test_thresholds_are_evaluated_again_when_reporting_resumes(self) -> None:
        _, events = evaluate_alarms([RULE], snapshot(10.0), state("above", stale=True), NOW, STALE_AFTER)

        self.assertEqual(kinds(events), ["reporting", "back below"])

    def test_unknown_sensor_is_stale_with_no_readings(self) -> None:
        new_state, events = evaluate_alarms([RULE], {}, state(), NOW, STALE_AFTER)

        self.assertEqual(new_state, state(stale=True))
        event = events[0]
        assert isinstance(event, FreshnessEvent)
        self.assertEqual((event.kind, event.last_observed_at), (FreshnessEventKind.STALE, None))

    def test_missing_property_on_a_fresh_sensor_holds_state(self) -> None:
        rule = AlarmRule("room", "air-pressure", above=1000.0, below=None, hysteresis=0.0)
        previous = AlarmState({rule.key: frozenset({"above"})}, frozenset())

        new_state, events = evaluate_alarms([rule], snapshot(20.0), previous, NOW, STALE_AFTER)

        self.assertEqual((new_state, events), (previous, []))

    def test_sensor_with_two_rules_reports_stale_once(self) -> None:
        other = AlarmRule("room", "relative-humidity", above=90.0, below=None, hysteresis=0.0)

        _, events = evaluate_alarms([RULE, other], {}, AlarmState.empty(), NOW, STALE_AFTER)

        self.assertEqual(kinds(events), ["stale"])


class StateCleanupTests(unittest.TestCase):
    def test_entries_for_removed_rules_are_dropped(self) -> None:
        previous = AlarmState(
            {RULE.key: frozenset({"above"}), "gone/air-pressure": frozenset({"below"})},
            frozenset({"gone"}),
        )

        new_state, events = evaluate_alarms([RULE], snapshot(36.0), previous, NOW, STALE_AFTER)

        self.assertEqual(new_state, state("above"))
        self.assertEqual(events, [])


class FormatTests(unittest.TestCase):
    def test_one_event_uses_the_plain_subject(self) -> None:
        _, events = evaluate_alarms([RULE], snapshot(36.2), state(), NOW, STALE_AFTER)

        notification = format_notification(events, NOW)

        self.assertEqual(notification.subject, "Weather station alarm")
        self.assertEqual(
            notification.body,
            "room temperature above 35.0 °C: now 36.2 °C (2026-10-21 14:05 EST)\n",
        )

    def test_every_event_kind_has_a_line(self) -> None:
        reading = AlarmReading("Temperature", "°C", 33.8, NOW)
        seen = NOW - timedelta(minutes=25)
        events = [
            ThresholdEvent(ThresholdEventKind.BACK_BELOW, "room", 34.0, reading),
            ThresholdEvent(ThresholdEventKind.BELOW, "room", 0.0, AlarmReading("Temperature", "°C", -1.5, NOW)),
            ThresholdEvent(ThresholdEventKind.BACK_ABOVE, "room", 1.0, AlarmReading("Temperature", "°C", 1.5, NOW)),
            FreshnessEvent(FreshnessEventKind.STALE, "room", seen),
            FreshnessEvent(FreshnessEventKind.STALE, "porch", None),
            FreshnessEvent(FreshnessEventKind.REPORTING, "room", NOW),
        ]

        notification = format_notification(events, NOW)

        self.assertEqual(notification.subject, "Weather station alarm: 6 events")
        self.assertEqual(
            notification.body.splitlines(),
            [
                "room temperature back below 34.0 °C: now 33.8 °C (2026-10-21 14:05 EST)",
                "room temperature below 0.0 °C: now -1.5 °C (2026-10-21 14:05 EST)",
                "room temperature back above 1.0 °C: now 1.5 °C (2026-10-21 14:05 EST)",
                "room stale: no reading since 2026-10-21 13:40 EST",
                "porch stale: no readings yet",
                "room reporting again: last reading 2026-10-21 14:05 EST",
            ],
        )

    def test_times_convert_to_the_local_zone(self) -> None:
        reading = AlarmReading("Temperature", "°C", 36.0, datetime(2026, 10, 21, 19, 3, tzinfo=timezone.utc))
        event = ThresholdEvent(ThresholdEventKind.ABOVE, "room", 35.0, reading)

        self.assertIn("(2026-10-21 14:03 EST)", format_notification([event], NOW).body)


if __name__ == "__main__":
    unittest.main()
