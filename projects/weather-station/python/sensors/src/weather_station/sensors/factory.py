from collections.abc import Callable, Mapping
from typing import Any

from weather_station.sensors.config import SensorConfig
from weather_station.sensors.contracts import Sensor
from weather_station.sensors.errors import InvalidSensorConfiguration

Builder = Callable[[str, dict[str, Any], "SensorFactory"], Sensor]


class SensorFactory:
    """Builds sensor adapters from configuration. Adding a model is one entry in the builder table."""

    def __init__(self, builders: Mapping[str, Builder] | None = None) -> None:
        self._builders = dict(builders) if builders is not None else dict(DEFAULT_BUILDERS)
        self._adcs: dict[int, Any] = {}

    @property
    def models(self) -> tuple[str, ...]:
        return tuple(sorted(self._builders))

    def build(self, config: SensorConfig) -> Sensor:
        builder = self._builders.get(config.model)
        if builder is None:
            raise InvalidSensorConfiguration(
                f"Sensor {config.sensor_id!r} has unknown model {config.model!r}. "
                f"Known models: {', '.join(self.models)}"
            )
        return builder(config.sensor_id, dict(config.options), self)

    def adc(self, address: int) -> Any:
        """One ADS1115 per I2C address, shared by every analog sensor wired to it."""
        if address not in self._adcs:
            from weather_station.sensors.adapters.analog import ADS1115

            self._adcs[address] = ADS1115(address=address)
        return self._adcs[address]


def _take(options: dict[str, Any], sensor_id: str, allowed: Mapping[str, str]) -> dict[str, Any]:
    """Renames config keys to constructor arguments and rejects keys the model doesn't accept."""
    unknown = set(options) - set(allowed)
    if unknown:
        raise InvalidSensorConfiguration(
            f"Sensor {sensor_id!r} has unknown options: {', '.join(sorted(unknown))}"
        )
    return {allowed[key]: value for key, value in options.items()}


def _am2302(sensor_id: str, options: dict[str, Any], factory: SensorFactory) -> Sensor:
    from weather_station.sensors.adapters.am2302 import AM2302Sensor

    return AM2302Sensor(sensor_id=sensor_id, **_take(options, sensor_id, {"gpio": "gpio_pin"}))


def _bme280(sensor_id: str, options: dict[str, Any], factory: SensorFactory) -> Sensor:
    from weather_station.sensors.adapters.bme280 import BME280Sensor

    return BME280Sensor(sensor_id=sensor_id, **_take(options, sensor_id, {"i2c_address": "address"}))


def _sgp30(sensor_id: str, options: dict[str, Any], factory: SensorFactory) -> Sensor:
    from weather_station.sensors.adapters.sgp30 import SGP30Sensor

    _take(options, sensor_id, {})
    return SGP30Sensor(sensor_id=sensor_id)


def _veml6075(sensor_id: str, options: dict[str, Any], factory: SensorFactory) -> Sensor:
    from weather_station.sensors.adapters.uv_light import VEML6075Sensor

    _take(options, sensor_id, {})
    return VEML6075Sensor(sensor_id=sensor_id)


def _bh1750(sensor_id: str, options: dict[str, Any], factory: SensorFactory) -> Sensor:
    from weather_station.sensors.adapters.uv_light import BH1750Sensor

    _take(options, sensor_id, {})
    return BH1750Sensor(sensor_id=sensor_id)


def _ds18b20(sensor_id: str, options: dict[str, Any], factory: SensorFactory) -> Sensor:
    from weather_station.sensors.adapters.ds18b20 import DS18B20Sensor

    return DS18B20Sensor(sensor_id=sensor_id, **_take(options, sensor_id, {"index": "sensor_index"}))


def _as3935(sensor_id: str, options: dict[str, Any], factory: SensorFactory) -> Sensor:
    from weather_station.sensors.adapters.as3935 import AS3935Sensor

    return AS3935Sensor(sensor_id=sensor_id, **_take(options, sensor_id, {"i2c_address": "address"}))


def _anemometer(sensor_id: str, options: dict[str, Any], factory: SensorFactory) -> Sensor:
    from weather_station.sensors.adapters.wind_rain import PulseAnemometer

    allowed = {
        "gpio": "gpio_pin",
        "meters_per_second_per_hz": "meters_per_second_per_hz",
        "gust_window_seconds": "gust_window_seconds",
    }
    return PulseAnemometer(sensor_id=sensor_id, **_take(options, sensor_id, allowed))


def _rain_gauge(sensor_id: str, options: dict[str, Any], factory: SensorFactory) -> Sensor:
    from weather_station.sensors.adapters.wind_rain import TippingBucketRainGauge

    allowed = {"gpio": "gpio_pin", "mm_per_tip": "mm_per_tip"}
    return TippingBucketRainGauge(sensor_id=sensor_id, **_take(options, sensor_id, allowed))


def _wind_vane(sensor_id: str, options: dict[str, Any], factory: SensorFactory) -> Sensor:
    from weather_station.sensors.adapters.analog import AnalogWindVane

    kwargs = _take(options, sensor_id, {"adc_address": "adc_address", "channel": "channel"})
    adc = factory.adc(kwargs.pop("adc_address", 0x48))
    return AnalogWindVane(adc, sensor_id=sensor_id, **kwargs)


def _soil_moisture(sensor_id: str, options: dict[str, Any], factory: SensorFactory) -> Sensor:
    from weather_station.sensors.adapters.analog import AnalogSoilMoisture

    allowed = {
        "adc_address": "adc_address",
        "channel": "channel",
        "dry_voltage": "dry_voltage",
        "wet_voltage": "wet_voltage",
    }
    kwargs = _take(options, sensor_id, allowed)
    adc = factory.adc(kwargs.pop("adc_address", 0x48))
    return AnalogSoilMoisture(adc, sensor_id=sensor_id, **kwargs)


DEFAULT_BUILDERS: Mapping[str, Builder] = {
    "am2302": _am2302,
    "bme280": _bme280,
    "sgp30": _sgp30,
    "veml6075": _veml6075,
    "bh1750": _bh1750,
    "ds18b20": _ds18b20,
    "as3935": _as3935,
    "anemometer": _anemometer,
    "rain-gauge": _rain_gauge,
    "wind-vane": _wind_vane,
    "soil-moisture": _soil_moisture,
}
