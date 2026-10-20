from __future__ import annotations

from datetime import datetime, timezone
from typing import TYPE_CHECKING, Callable

from weather_station.sensors.errors import InvalidSensorConfiguration
from weather_station.shared.measurements import Measurement, make_measurement
from weather_station.shared.registry import ADC_VOLTAGE, SOIL_MOISTURE, WIND_DIRECTION

if TYPE_CHECKING:
    from adafruit_ads1x15.analog_in import AnalogIn


class ADS1115:
    def __init__(self, address: int = 0x48):
        import adafruit_ads1x15.ads1115 as ADS
        import board

        self.i2c = board.I2C()
        self.device = ADS.ADS1115(self.i2c, address=address)
        self.device.gain = 1

    def channel(self, channel: int) -> AnalogIn:
        import adafruit_ads1x15.ads1115 as ADS
        from adafruit_ads1x15.analog_in import AnalogIn

        if not 0 <= channel <= 3:
            raise InvalidSensorConfiguration("ADS1115 channel must be between 0 and 3")
        return AnalogIn(self.device, getattr(ADS, f"P{channel}"))


class AnalogWindVane:
    """Requires calibration: map ADC voltage to degrees for your particular vane."""

    sensor_type = "wind"

    def __init__(
        self,
        adc: ADS1115,
        channel: int = 0,
        sensor_id: str = "wind-vane",
        voltage_to_degrees: Callable[[float], float] | None = None,
    ):
        self.adc, self.channel, self.sensor_id = adc, channel, sensor_id
        self.input = adc.channel(channel)
        self.voltage_to_degrees = voltage_to_degrees or (lambda voltage: 0.0)

    def read(self) -> list[Measurement]:
        voltage = float(self.input.voltage)
        direction = float(self.voltage_to_degrees(voltage)) % 360.0
        timestamp = datetime.now(timezone.utc)
        return [
            make_measurement(
                self.sensor_id,
                self.sensor_type,
                WIND_DIRECTION.property_id,
                direction * 0.017453292519943295,
                WIND_DIRECTION.si_unit,
                timestamp,
            ),
            make_measurement(
                self.sensor_id,
                self.sensor_type,
                ADC_VOLTAGE.property_id,
                voltage,
                ADC_VOLTAGE.si_unit,
                timestamp,
            ),
        ]

    def close(self) -> None:
        pass


class AnalogSoilMoisture:
    """Calibrate dry_voltage and wet_voltage for the exact probe and soil."""

    sensor_type = "soil_moisture"

    def __init__(
        self,
        adc: ADS1115,
        channel: int = 1,
        sensor_id: str = "soil-moisture",
        dry_voltage: float = 2.8,
        wet_voltage: float = 1.2,
    ):
        self.input = adc.channel(channel)
        self.sensor_id = sensor_id
        self.dry_voltage = dry_voltage
        self.wet_voltage = wet_voltage

    def read(self) -> list[Measurement]:
        voltage = float(self.input.voltage)
        span = self.dry_voltage - self.wet_voltage
        percent = 0.0 if span == 0 else (self.dry_voltage - voltage) / span * 100.0
        percent = max(0.0, min(100.0, percent))
        timestamp = datetime.now(timezone.utc)
        return [
            make_measurement(
                self.sensor_id,
                self.sensor_type,
                SOIL_MOISTURE.property_id,
                percent / 100,
                SOIL_MOISTURE.si_unit,
                timestamp,
            ),
            make_measurement(
                self.sensor_id,
                self.sensor_type,
                ADC_VOLTAGE.property_id,
                voltage,
                ADC_VOLTAGE.si_unit,
                timestamp,
            ),
        ]

    def close(self) -> None:
        pass
