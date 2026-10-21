# Wire the AM2302

The shipped `config.example.json` connects the AM2302/DHT22 data line to BCM GPIO 4 (physical pin 7).

| AM2302 | Raspberry Pi               |
| ------ | -------------------------- |
| Data   | BCM GPIO 4, physical pin 7 |
| VCC    | 3.3 V                      |
| GND    | Ground                     |

If the sensor breakout doesn't include a pull-up, add 4.7 kΩ between data and 3.3 V. Never connect a 5 V signal to a Pi GPIO. Check the board datasheet for its exact supply and pull-up requirements; the reference datasheet is in `docs/datasheets/AM2302-EN.pdf`.

The user running `weather-sensors.service` needs GPIO access. For Raspberry Pi OS-level GPIO setup, follow the current [Adafruit CircuitPython DHT guide](https://docs.circuitpython.org/projects/dht/en/latest/).
