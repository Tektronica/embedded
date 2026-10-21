from dataclasses import dataclass
from datetime import timedelta

from weather_station.shared.units import UnitSystem, unit_system_for_code

UNIT_COOKIE = "unit"
UNIT_COOKIE_MAX_AGE = timedelta(days=365)


@dataclass(frozen=True)
class UnitChoice:
    system: UnitSystem
    from_query: bool


def resolve_unit(query_code: str | None, cookie_code: str | None, default: UnitSystem) -> UnitChoice:
    """Query beats cookie beats config default. An unknown code is skipped."""
    queried = unit_system_for_code(query_code)
    if queried is not None:
        return UnitChoice(queried, from_query=True)
    return UnitChoice(unit_system_for_code(cookie_code) or default, from_query=False)
