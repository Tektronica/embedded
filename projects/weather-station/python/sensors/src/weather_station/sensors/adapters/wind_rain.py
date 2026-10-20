import threading
import time
from datetime import datetime, timezone
from gpiozero import Button
from weather_station.shared.measurements import Measurement, make_measurement
from weather_station.shared.registry import (
    RAINFALL_ACCUMULATION,
    RAINFALL_RATE,
    WIND_GUST,
    WIND_SPEED,
)


class PulseAnemometer:
    """Generic reed/Hall pulse anemometer. Set m/s per pulse per second from its datasheet."""

    sensor_type = "wind"

    def __init__(
        self,
        gpio_pin: int = 17,
        sensor_id: str = "wind-speed",
        meters_per_second_per_hz: float = 2.4,
        gust_window_seconds: int = 300,
    ):
        self.sensor_id = sensor_id
        self.factor = meters_per_second_per_hz
        self.gust_window_seconds = gust_window_seconds
        self._lock = threading.Lock()
        self._pulses = 0
        self._samples: list[tuple[float, float]] = []
        self.button = Button(gpio_pin, pull_up=True, bounce_time=0.005)
        self.button.when_pressed = self._pulse
        self._last_read = time.monotonic()

    def _pulse(self) -> None:
        with self._lock:
            self._pulses += 1

    def read(self) -> list[Measurement]:
        now = time.monotonic()
        elapsed = max(now - self._last_read, 0.001)
        with self._lock:
            pulses = self._pulses
            self._pulses = 0
        self._last_read = now
        speed = pulses / elapsed * self.factor
        self._samples.append((now, speed))
        self._samples = [
            (t, s) for t, s in self._samples if now - t <= self.gust_window_seconds
        ]
        gust = max((s for _, s in self._samples), default=speed)
        timestamp = datetime.now(timezone.utc)
        return [
            make_measurement(
                self.sensor_id,
                self.sensor_type,
                WIND_SPEED.property_id,
                speed,
                WIND_SPEED.si_unit,
                timestamp,
            ),
            make_measurement(
                self.sensor_id,
                self.sensor_type,
                WIND_GUST.property_id,
                gust,
                WIND_GUST.si_unit,
                timestamp,
            ),
        ]

    def close(self) -> None:
        self.button.close()


class TippingBucketRainGauge:
    """Generic reed-switch tipping bucket; set mm_per_tip from gauge calibration."""

    sensor_type = "rainfall"

    def __init__(
        self,
        gpio_pin: int = 27,
        sensor_id: str = "rainfall",
        mm_per_tip: float = 0.2794,
    ):
        self.sensor_id = sensor_id
        self.mm_per_tip = mm_per_tip
        self._lock = threading.Lock()
        self._tips = 0
        self._total_mm = 0.0
        self._last_read = time.monotonic()
        self.button = Button(gpio_pin, pull_up=True, bounce_time=0.05)
        self.button.when_pressed = self._tip

    def _tip(self) -> None:
        with self._lock:
            self._tips += 1
            self._total_mm += self.mm_per_tip

    def read(self) -> list[Measurement]:
        now = time.monotonic()
        elapsed_hours = max((now - self._last_read) / 3600.0, 1e-9)
        with self._lock:
            tips = self._tips
            self._tips = 0
            total = self._total_mm
        self._last_read = now
        rate_m_per_second = tips * self.mm_per_tip / elapsed_hours / 3_600_000
        timestamp = datetime.now(timezone.utc)
        return [
            make_measurement(
                self.sensor_id,
                self.sensor_type,
                RAINFALL_ACCUMULATION.property_id,
                total / 1000,
                RAINFALL_ACCUMULATION.si_unit,
                timestamp,
            ),
            make_measurement(
                self.sensor_id,
                self.sensor_type,
                RAINFALL_RATE.property_id,
                rate_m_per_second,
                RAINFALL_RATE.si_unit,
                timestamp,
            ),
        ]

    def close(self) -> None:
        self.button.close()
