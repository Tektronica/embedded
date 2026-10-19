#pragma once

#include <stdint.h>
#include <RF24.h>
#include <SPI.h>

// Thin driver around an nRF24L01+ radio: owns the RF24 instance and a fixed writing/reading pipe
// pair, exposing simple "power up/down, transmit a message, receive a message with a timeout"
// semantics. Generic -- knows nothing about what's being sent (see Payload.h for the wire format
// this project sends, and main.cpp for when it's used). Templated on message type since transmit
// and receive here genuinely carry two different types (an outgoing Telemetry struct, an
// incoming raw UTC value) -- not a single-call-site abstraction. Hardware-coupled (SPI), so this
// doesn't unit-test off-device.
namespace radio {

class Link {
 public:
  Link(uint8_t cePin, uint8_t csnPin) : radio_(cePin, csnPin) {}

  void begin(uint64_t writePipe, uint64_t readPipe, uint8_t maxRetries) {
    radio_.begin();
    radio_.setPALevel(RF24_PA_MIN);
    radio_.setDataRate(RF24_250KBPS);
    radio_.setRetries(10, maxRetries);  // 10 * 250us delay between retries
    radio_.openWritingPipe(writePipe);
    radio_.openReadingPipe(1, readPipe);
  }

  void wake() { radio_.powerUp(); }
  void sleep() { radio_.powerDown(); }

  // Transmits message if a chip is actually present; otherwise a no-op. RF24::write() blocks
  // waiting for the chip's own TX_DS/MAX_RT status bits, which never arrive without real
  // hardware on the bus -- checking first avoids hanging the whole dive cycle on a missing or
  // failed radio (in the Wokwi simulator, where the nRF24L01+ isn't simulated at all, or on real
  // hardware if the radio ever disconnects).
  template <typename T>
  void transmit(const T& message) {
    if (!radio_.isChipConnected()) return;
    radio_.stopListening();
    delay(5);  // radio needs a moment to switch to TX mode
    radio_.write(&message, sizeof(message));
  }

  // Waits up to timeoutMs for a message; returns true and fills out if one arrived. Returns
  // immediately if no chip is present, same reasoning as transmit().
  template <typename T>
  bool receive(T& out, uint32_t timeoutMs) {
    if (!radio_.isChipConnected()) return false;
    radio_.startListening();
    uint32_t waitStartMs = millis();
    while (millis() - waitStartMs < timeoutMs) {
      if (radio_.available()) {
        radio_.read(&out, sizeof(out));
        return true;
      }
    }
    return false;
  }

 private:
  RF24 radio_;
};

}  // namespace radio
