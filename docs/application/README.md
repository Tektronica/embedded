# Application example: toy-kitchen cooktop

This is *one example* of using the generic [LED-strip dimmer controller](../../). The firmware itself
has no kitchen concept — it maps N dimmer inputs to N WS2812 LED strip outputs. Here that mechanism is
arranged as a play-kitchen **cooktop**, where the application vocabulary lives (and nowhere else).

## The mapping

| Generic mechanism | In this application |
|---|---|
| channel (0–3) | a hob burner |
| dimmer input | the burner's control knob |
| WS2812 LED strip | the burner's ring of LEDs |
| `level` (0–255) | how "hot" the burner is |
| `levelColor` curve | the heat glow (off → deep red → orange-red) |

## The build

- 4 channels → 4 hob burners (`CHANNEL_COUNT = 4`).
- Each burner is a pre-made **35-LED WS2812 ring** (5 V, 5050 RGB with integrated drivers;
  `LED_STRIP_LENGTH = 35`). To the firmware a ring is just 35 sequential pixels; the round shape is
  enclosure-only.
- 4 knobs (dimmers) on the kitchen's front panel, one per burner.
- Turning a knob ramps its burner off → deep red → orange-red, like a heating element.

## Notes

- Nothing here changes the firmware — it's enclosure/wiring plus the default `levelColor` curve.
- A different application (mood lighting, a VU meter, a thermostat display, …) reuses the same build
  with a different curve and a different enclosure.
