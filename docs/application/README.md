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
- Each WS2812 LED strip is bent into a ring under a translucent burner (`LED_STRIP_LENGTH = 35`).
- 4 knobs (dimmers) on the kitchen's front panel, one per burner.
- Turning a knob ramps its burner off → deep red → orange-red, like a heating element.

## Notes

- Nothing here changes the firmware — it's enclosure/wiring plus the default `levelColor` curve.
- A different application (mood lighting, a VU meter, a thermostat display, …) reuses the same build
  with a different curve and a different enclosure.
