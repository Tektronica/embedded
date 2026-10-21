import json
from pathlib import Path
from typing import Any

from dotenv import find_dotenv, load_dotenv

from weather_station.shared.errors import (
    ConfigFileError,
    InvalidDisplayConfiguration,
    InvalidStatusConfiguration,
    InvalidStorageConfiguration,
    WeatherStationError,
)
from weather_station.shared.units import CELSIUS, UNIT_SYSTEMS, UnitSystem

DEFAULT_CONFIG_PATH = Path("config.json")
DEFAULT_DATABASE_PATH = "instance/weather.sqlite3"
DEFAULT_STALE_AFTER_MINUTES = 5
CONFIG_SECTIONS = frozenset({"storage", "sensors", "status", "display", "email", "alarms"})
STATUS_KEYS = frozenset({"stale_after_minutes"})
DISPLAY_KEYS = frozenset({"temperature_unit"})


def load_env() -> None:
    """Loads .env from the working directory or its nearest parent. Variables already set win."""
    load_dotenv(find_dotenv(usecwd=True))


def load_config(path: Path = DEFAULT_CONFIG_PATH) -> dict[str, Any]:
    if not path.exists():
        raise ConfigFileError(f"No configuration at {path}. Copy config.example.json to {path.name}.")
    try:
        with path.open(encoding="utf-8") as file:
            document = json.load(file)
    except json.JSONDecodeError as error:
        raise ConfigFileError(f"Invalid JSON in {path}: {error}") from error
    if not isinstance(document, dict):
        raise ConfigFileError(f"{path} must hold a JSON object at the top level")
    unknown = set(document) - CONFIG_SECTIONS
    if unknown:
        raise ConfigFileError(f"{path} has unknown sections: {', '.join(sorted(unknown))}")
    return document


def read_section(
    document: dict[str, Any],
    name: str,
    allowed_keys: frozenset[str],
    error: type[WeatherStationError],
) -> dict[str, Any]:
    section = document.get(name, {})
    if not isinstance(section, dict):
        raise error(f"Config section {name!r} must be an object")
    unknown = set(section) - allowed_keys
    if unknown:
        raise error(f"Config section {name!r} has unknown keys: {', '.join(sorted(unknown))}")
    return section


def read_int(
    section: dict[str, Any],
    section_name: str,
    key: str,
    default: int,
    error: type[WeatherStationError],
) -> int:
    value = section.get(key, default)
    if not isinstance(value, int) or isinstance(value, bool):
        raise error(f"Config key '{section_name}.{key}' must be an integer")
    return value


def read_positive_int(
    section: dict[str, Any],
    section_name: str,
    key: str,
    default: int,
    error: type[WeatherStationError],
) -> int:
    value = read_int(section, section_name, key, default, error)
    if value <= 0:
        raise error(f"Config key '{section_name}.{key}' must be positive")
    return value


def database_path(config_path: Path = DEFAULT_CONFIG_PATH) -> Path:
    section = read_section(
        load_config(config_path), "storage", frozenset({"database_path"}), InvalidStorageConfiguration
    )
    value = section.get("database_path", DEFAULT_DATABASE_PATH)
    if not isinstance(value, str) or not value:
        raise InvalidStorageConfiguration("Config key 'storage.database_path' must be a non-empty string")
    return Path(value)


def read_stale_after_minutes(document: dict[str, Any]) -> int:
    status = read_section(document, "status", STATUS_KEYS, InvalidStatusConfiguration)
    return read_positive_int(
        status, "status", "stale_after_minutes", DEFAULT_STALE_AFTER_MINUTES, InvalidStatusConfiguration
    )


def read_unit_system(document: dict[str, Any]) -> UnitSystem:
    display = read_section(document, "display", DISPLAY_KEYS, InvalidDisplayConfiguration)
    key = display.get("temperature_unit", CELSIUS.key)
    if not isinstance(key, str) or key not in UNIT_SYSTEMS:
        allowed = ", ".join(f'"{name}"' for name in UNIT_SYSTEMS)
        raise InvalidDisplayConfiguration(f"Config key 'display.temperature_unit' must be one of {allowed}")
    return UNIT_SYSTEMS[key]
