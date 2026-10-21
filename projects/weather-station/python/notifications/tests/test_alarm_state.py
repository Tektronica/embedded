import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from weather_station.notifications.alarm_state import AlarmState, AlarmStateStore


class AlarmStateStoreTests(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = TemporaryDirectory()
        self.path = Path(self.directory.name) / "instance" / "alarm-state.json"
        self.store = AlarmStateStore(self.path)

    def tearDown(self) -> None:
        self.directory.cleanup()

    def test_round_trip(self) -> None:
        state = AlarmState({"sensor-am2302/air-temperature": frozenset({"above"})}, frozenset({"sensor-bme280"}))

        self.store.save(state)

        self.assertEqual(self.store.load(), state)
        self.assertEqual([item.name for item in self.path.parent.iterdir()], ["alarm-state.json"])

    def test_missing_file_starts_empty_with_a_warning(self) -> None:
        with self.assertLogs(level="WARNING"):
            self.assertEqual(self.store.load(), AlarmState.empty())

    def test_corrupt_file_starts_empty_with_a_warning(self) -> None:
        self.path.parent.mkdir()
        for content in ("{not json", "[]", '{"thresholds": {"a": ["sideways"]}, "stale": []}', '{"thresholds": {}}'):
            with self.subTest(content=content):
                self.path.write_text(content)
                with self.assertLogs(level="WARNING"):
                    self.assertEqual(self.store.load(), AlarmState.empty())


if __name__ == "__main__":
    unittest.main()
