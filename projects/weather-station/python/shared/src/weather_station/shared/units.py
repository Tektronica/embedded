from collections.abc import Mapping
from dataclasses import dataclass
from types import MappingProxyType

from weather_station.shared.registry import QUANTITY_KIND, PropertyDefinition


@dataclass(frozen=True)
class UnitConversion:
    unit: str
    scale: float
    offset: float

    def apply(self, si_value: float) -> float:
        return si_value * self.scale + self.offset


@dataclass(frozen=True)
class UnitSystem:
    """Converts stored SI values for presentation. A quantity kind with no conversion keeps the property's own display unit."""

    key: str
    code: str
    temperature_symbol: str
    conversions: Mapping[str, UnitConversion]

    def value(self, prop: PropertyDefinition, si_value: float) -> float:
        conversion = self.conversions.get(prop.quantity_kind_uri)
        return conversion.apply(si_value) if conversion else prop.display_value(si_value)

    def unit(self, prop: PropertyDefinition) -> str:
        conversion = self.conversions.get(prop.quantity_kind_uri)
        return conversion.unit if conversion else prop.display_unit


CELSIUS = UnitSystem("celsius", "c", "°C", MappingProxyType({}))
FAHRENHEIT = UnitSystem(
    "fahrenheit",
    "f",
    "°F",
    MappingProxyType({f"{QUANTITY_KIND}Temperature": UnitConversion("°F", 9 / 5, -273.15 * 9 / 5 + 32)}),
)

UNIT_SYSTEMS = MappingProxyType({system.key: system for system in (CELSIUS, FAHRENHEIT)})


def unit_system_for_code(code: str | None) -> UnitSystem | None:
    return next((system for system in UNIT_SYSTEMS.values() if system.code == code), None)
