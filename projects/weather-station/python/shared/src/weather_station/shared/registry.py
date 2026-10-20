from dataclasses import dataclass
from types import MappingProxyType

from weather_station.shared.errors import InvalidMeasurementUnit, UnknownProperty, UnknownSensorType
from weather_station.shared.measurements import Measurement

QUDT_UNIT = "http://qudt.org/vocab/unit/"
PROPERTY = "urn:weather-station:property:"
QUANTITY_KIND = "http://qudt.org/vocab/quantitykind/"


@dataclass(frozen=True)
class PropertyDefinition:
    key: str
    property_id: str
    label: str
    quantity_kind_uri: str
    si_unit: str
    display_unit: str
    display_scale: float = 1.0
    display_offset: float = 0.0

    def display_value(self, value: float) -> float:
        return value * self.display_scale + self.display_offset


@dataclass(frozen=True)
class SensorDefinition:
    key: str
    label: str
    properties: tuple[PropertyDefinition, ...]


def _property(
    key: str,
    label: str,
    quantity_kind: str,
    unit: str,
    display_unit: str,
    scale: float = 1.0,
    offset: float = 0.0,
) -> PropertyDefinition:
    return PropertyDefinition(
        key=key,
        property_id=f"{PROPERTY}{key}",
        label=label,
        quantity_kind_uri=f"{QUANTITY_KIND}{quantity_kind}",
        si_unit=f"{QUDT_UNIT}{unit}",
        display_unit=display_unit,
        display_scale=scale,
        display_offset=offset,
    )


TEMPERATURE = _property("air-temperature", "Temperature", "Temperature", "K", "°C", offset=-273.15)
RELATIVE_HUMIDITY = _property("relative-humidity", "Relative humidity", "RelativeHumidity", "UNITLESS", "%", 100)
PRESSURE = _property("air-pressure", "Pressure", "Pressure", "PA", "hPa", 0.01)
TVOC = _property("tvoc", "TVOC", "MoleFraction", "MOL-PER-MOL", "ppm", 1_000_000)
EQUIVALENT_CO2 = _property("equivalent-co2", "Equivalent CO₂", "MoleFraction", "MOL-PER-MOL", "ppm", 1_000_000)
WIND_SPEED = _property("wind-speed", "Wind speed", "Speed", "M-PER-SEC", "m/s")
WIND_GUST = _property("wind-gust", "Gust", "Speed", "M-PER-SEC", "m/s")
WIND_DIRECTION = _property("wind-direction", "Direction", "Angle", "RAD", "°", 57.2957795131)
ADC_VOLTAGE = _property("adc-voltage", "ADC voltage", "Voltage", "V", "V")
RAINFALL_ACCUMULATION = _property("rainfall-accumulation", "Accumulation", "Length", "M", "mm", 1000)
RAINFALL_RATE = _property("rainfall-rate", "Rate", "Velocity", "M-PER-SEC", "mm/h", 3_600_000)
UV_INDEX = _property("uv-index", "UV index", "DimensionlessRatio", "UNITLESS", "")
ILLUMINANCE = _property("illuminance", "Illuminance", "Illuminance", "LUX", "lx")
SOIL_MOISTURE = _property("soil-moisture", "Moisture", "DimensionlessRatio", "UNITLESS", "%", 100)
LIGHTNING_COUNT = _property("lightning-count", "Interrupts", "Count", "NUM", "events")
LIGHTNING_DISTANCE = _property("lightning-distance", "Last distance", "Length", "M", "km", 0.001)

SENSOR_DEFINITIONS = MappingProxyType({
    definition.key: definition
    for definition in (
        SensorDefinition("temperature_humidity", "Temperature and humidity", (TEMPERATURE, RELATIVE_HUMIDITY)),
        SensorDefinition("pressure", "Pressure", (TEMPERATURE, RELATIVE_HUMIDITY, PRESSURE)),
        SensorDefinition("air_quality", "Air quality", (TVOC, EQUIVALENT_CO2)),
        SensorDefinition("wind", "Wind", (WIND_SPEED, WIND_GUST, WIND_DIRECTION, ADC_VOLTAGE)),
        SensorDefinition("rainfall", "Rainfall", (RAINFALL_ACCUMULATION, RAINFALL_RATE)),
        SensorDefinition("uv", "UV", (UV_INDEX,)),
        SensorDefinition("light", "Illuminance", (ILLUMINANCE,)),
        SensorDefinition("probe_temperature", "Probe temperature", (TEMPERATURE,)),
        SensorDefinition("soil_moisture", "Soil moisture", (SOIL_MOISTURE, ADC_VOLTAGE)),
        SensorDefinition("lightning", "Lightning", (LIGHTNING_COUNT, LIGHTNING_DISTANCE)),
    )
})


def get_sensor_definition(sensor_type: str) -> SensorDefinition:
    try:
        return SENSOR_DEFINITIONS[sensor_type]
    except KeyError as error:
        raise UnknownSensorType(sensor_type) from error


def get_property_definition(sensor_type: str, property_id: str) -> PropertyDefinition:
    definition = get_sensor_definition(sensor_type)
    for item in definition.properties:
        if item.property_id == property_id:
            return item
    raise UnknownProperty(f"Property {property_id!r} is not supported by sensor type {sensor_type!r}")


def validate_measurement(sensor_type: str, measurement: Measurement) -> Measurement:
    property_definition = get_property_definition(sensor_type, measurement.property_id)
    if measurement.unit != property_definition.si_unit:
        raise InvalidMeasurementUnit(
            f"Property {measurement.property_id!r} requires unit {property_definition.si_unit!r}"
        )
    return measurement
