"""Enforces the package boundaries: shared imports none of the others, and sensors, web, and
notifications import only shared, never each other."""

import ast
import unittest
from pathlib import Path

PYTHON_ROOT = Path(__file__).resolve().parents[1]
PACKAGES = ("shared", "sensors", "web", "notifications")
ALLOWED = {
    "shared": {"shared"},
    "sensors": {"sensors", "shared"},
    "web": {"web", "shared"},
    "notifications": {"notifications", "shared"},
}
HARDWARE_MODULES = {"board", "gpiozero", "w1thermsensor", "adafruit_dht"}


def imported_modules(package: str) -> set[str]:
    modules: set[str] = set()
    for path in (PYTHON_ROOT / package).rglob("*.py"):
        tree = ast.parse(path.read_text(encoding="utf-8"))
        for node in ast.walk(tree):
            if isinstance(node, ast.Import):
                modules.update(alias.name for alias in node.names)
            elif isinstance(node, ast.ImportFrom) and node.module:
                modules.add(node.module)
    return modules


class PackageBoundaryTests(unittest.TestCase):
    def test_each_package_imports_only_itself_and_shared(self) -> None:
        for package in PACKAGES:
            used = {
                module.split(".")[1]
                for module in imported_modules(package)
                if module.startswith("weather_station.")
            }
            with self.subTest(package=package):
                self.assertLessEqual(used, ALLOWED[package])

    def test_hardware_libraries_stay_in_sensors(self) -> None:
        for package in ("shared", "web", "notifications"):
            roots = {module.split(".")[0] for module in imported_modules(package)}
            with self.subTest(package=package):
                self.assertFalse(roots & HARDWARE_MODULES)
                self.assertFalse({root for root in roots if root.startswith("adafruit")})

    def test_flask_stays_in_web(self) -> None:
        for package in ("shared", "sensors", "notifications"):
            roots = {module.split(".")[0] for module in imported_modules(package)}
            with self.subTest(package=package):
                self.assertNotIn("flask", roots)


if __name__ == "__main__":
    unittest.main()
