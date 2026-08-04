#pragma once

#include <stdint.h>
#include <Arduino.h>

#include "ET6226MCodec.h"

// Driver for the UMW ET6226M: a combined LED matrix drive + 7x4 keyboard scan controller. No
// Arduino library exists for this chip, so this is a from-scratch driver built off the
// datasheet's two-wire CLK/DAT protocol -- shaped like TM1637's (start/stop framing, byte+ACK),
// but not the same command set or bit order (this one is MSB-first; TM1637 is LSB-first), so it
// can't reuse that library. Hardware-coupled (digitalWrite/digitalRead), so unlike
// ET6226MCodec.h this doesn't unit-test off-device.
//
// The API stays centered on the chip's own model -- grids (1-4), segments (1-7, plus DP/KP as an
// 8th bit), and raw key codes -- rather than anything about what's driving it, since that's what
// the datasheet itself is organized around. A consuming project's own concepts belong in that
// project's code, not here; see README.md's "Open questions" for what's still unconfirmed against
// the full datasheet.
class ET6226M {
 public:
  static constexpr uint8_t GRID_COUNT = 4;

  ET6226M(uint8_t pinClk, uint8_t pinDat) : pinClk_(pinClk), pinDat_(pinDat) {}

  // Configures CLK/DAT pin modes, idles the bus, and sends an initial Display Control Command
  // (full brightness, display on) so the chip is actually driving the display before the first
  // setGrid()/setSegments() call. Call once from setup().
  void begin() {
    pinMode(pinClk_, OUTPUT);
    digitalWrite(pinClk_, HIGH);
    releaseDat();
    writeDisplayControl();
  }

  // Brightness, 0 (dimmest) to et6226m::MAX_BRIGHTNESS (brightest); out-of-range values clamp
  // rather than wrap, so a caller experimenting with the range can't accidentally darken the
  // display by overshooting.
  void setBrightness(uint8_t level) {
    brightness_ = level;
    writeDisplayControl();
  }

  // Blanks or restores the display without touching any grid's segment data, so turning it back
  // on shows whatever was last written rather than needing a re-send.
  void setDisplayOn(bool on) {
    displayOn_ = on;
    writeDisplayControl();
  }

  // Writes one grid's (1-4) raw segment byte -- bit7=DP/KP, bit6..0=SG7..SG1, per the datasheet's
  // Display Data Command table. Out-of-range grid numbers are a no-op rather than an out-of-
  // bounds array read. Each grid is its own start/command/data/stop transaction; the excerpt
  // this was built from didn't document an auto-increment burst mode the way TM1637 has one.
  void setGrid(uint8_t grid, uint8_t segmentByte) {
    if (grid < 1 || grid > GRID_COUNT) return;
    static constexpr Command GRID_COMMANDS[GRID_COUNT] = {Command::Grid1, Command::Grid2,
                                                            Command::Grid3, Command::Grid4};
    start();
    writeByte(static_cast<uint8_t>(GRID_COMMANDS[grid - 1]));
    writeByte(segmentByte);
    stop();
  }

  // Writes all 4 grids in one call, GR1 first.
  void setSegments(const uint8_t segments[GRID_COUNT]) {
    for (uint8_t i = 0; i < GRID_COUNT; ++i) setGrid(i + 1, segments[i]);
  }

  // Raw key code from the Key Code Command; 0x00 if no key is currently pressed, or if the
  // command byte itself wasn't ACKed (an unresponsive bus shouldn't be reported as "no key" via
  // a floating read -- treating a NACK as "no key" is the same outcome without pretending we
  // actually read anything meaningful). Decode with et6226m::decodeKeyCode() (ET6226MCodec.h).
  uint8_t readKeyCode() {
    start();
    bool acked = writeByte(static_cast<uint8_t>(Command::ReadKey));
    uint8_t code = acked ? readByte() : 0x00;
    stop();
    return code;
  }

 private:
  enum class Command : uint8_t {
    Grid1 = 0x68,
    Grid2 = 0x6A,
    Grid3 = 0x6C,
    Grid4 = 0x6E,
    ReadKey = 0x4F,
    DisplayControl = 0x48,
  };

  static constexpr uint16_t BUS_DELAY_US = 3;  // placeholder -- datasheet gave no max CLK freq

  // Sends the current brightness/on-off state via the Display Control Command. Both
  // setBrightness() and setDisplayOn() go through this rather than a partial update, since the
  // chip takes one combined byte for both settings (see et6226m::encodeDisplayControl).
  void writeDisplayControl() {
    start();
    writeByte(static_cast<uint8_t>(Command::DisplayControl));
    writeByte(et6226m::encodeDisplayControl(brightness_, displayOn_));
    stop();
  }

  // DAT is open-drain per the datasheet ("built-in drain mode pull-on"): this driver never
  // actively drives it high, only ever pulls it low or releases it to be pulled up (the AVR's
  // own INPUT_PULLUP, backing up whatever pull-up the chip/board provides) -- driving it high
  // with a push-pull OUTPUT would fight the chip if it's simultaneously trying to pull DAT low
  // (e.g. during its own ACK). CLK has no such note in the datasheet and only this driver ever
  // drives it (no clock-stretching is documented), so it stays a plain push-pull output.
  void releaseDat() { pinMode(pinDat_, INPUT_PULLUP); }
  void pullDatLow() {
    pinMode(pinDat_, OUTPUT);
    digitalWrite(pinDat_, LOW);
  }

  void start() {
    digitalWrite(pinClk_, HIGH);
    releaseDat();
    delayMicroseconds(BUS_DELAY_US);
    pullDatLow();  // DAT falls while CLK high -- start condition
    delayMicroseconds(BUS_DELAY_US);
    digitalWrite(pinClk_, LOW);
  }

  void stop() {
    digitalWrite(pinClk_, LOW);
    pullDatLow();
    delayMicroseconds(BUS_DELAY_US);
    digitalWrite(pinClk_, HIGH);
    delayMicroseconds(BUS_DELAY_US);
    releaseDat();  // DAT rises while CLK high -- stop condition
    delayMicroseconds(BUS_DELAY_US);
  }

  // MSB-first, per the datasheet ("maximum signal bit headed sent"). Returns whether the chip
  // ACKed (pulled DAT low on the ninth clock) -- readKeyCode() uses this to avoid trusting a
  // read from a bus nothing responded on; setGrid() doesn't check it since a dropped display
  // write just means one frame doesn't show, self-correcting on the next refresh.
  bool writeByte(uint8_t value) {
    for (int8_t bit = 7; bit >= 0; --bit) {
      digitalWrite(pinClk_, LOW);
      if ((value >> bit) & 0x01) {
        releaseDat();
      } else {
        pullDatLow();
      }
      delayMicroseconds(BUS_DELAY_US);
      digitalWrite(pinClk_, HIGH);
      delayMicroseconds(BUS_DELAY_US);
    }
    digitalWrite(pinClk_, LOW);
    releaseDat();  // let the chip pull DAT low to ACK
    delayMicroseconds(BUS_DELAY_US);
    digitalWrite(pinClk_, HIGH);
    delayMicroseconds(BUS_DELAY_US);
    bool acked = digitalRead(pinDat_) == LOW;
    digitalWrite(pinClk_, LOW);
    return acked;
  }

  // MSB-first; DAT stays released (input) throughout since the chip drives it, and is sampled
  // while CLK is high, matching the datasheet's rule that DAT only changes while CLK is low. No
  // master ACK/NACK is sent afterward -- the excerpt this was built from didn't document that
  // step for a read.
  uint8_t readByte() {
    releaseDat();
    uint8_t value = 0;
    for (int8_t bit = 7; bit >= 0; --bit) {
      digitalWrite(pinClk_, LOW);
      delayMicroseconds(BUS_DELAY_US);
      digitalWrite(pinClk_, HIGH);
      delayMicroseconds(BUS_DELAY_US);
      if (digitalRead(pinDat_) == HIGH) value |= static_cast<uint8_t>(1 << bit);
    }
    digitalWrite(pinClk_, LOW);
    return value;
  }

  uint8_t pinClk_;
  uint8_t pinDat_;
  uint8_t brightness_ = et6226m::MAX_BRIGHTNESS;
  bool displayOn_ = true;
};
