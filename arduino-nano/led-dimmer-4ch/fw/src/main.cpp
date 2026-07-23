#include <Arduino.h>

#include "Pwm.h"

namespace {

// Pin assignments: CH0..3 -> D11/D10/D9/D3, potentiometers -> A0..A3. D9/D10 (Timer1) and D11/D3
// (Timer2) share the same default ~490 Hz Phase-Correct PWM under analogWrite(), so all 4
// channels run at one shared frequency with zero timer register setup.
constexpr uint8_t PIN_POT[pwm::CHANNEL_COUNT] = {A0, A1, A2, A3};
constexpr uint8_t PIN_PWM[pwm::CHANNEL_COUNT] = {11, 10, 9, 3};

constexpr uint8_t EMA_SHIFT = 3;  // pot smoothing strength

uint16_t smoothed[pwm::CHANNEL_COUNT];

}  // namespace

void setup() {
  for (uint8_t ch = 0; ch < pwm::CHANNEL_COUNT; ++ch) {
    pinMode(PIN_POT[ch], INPUT);
    pinMode(PIN_PWM[ch], OUTPUT);
    smoothed[ch] = analogRead(PIN_POT[ch]);  // start at each pot's actual position
  }
}

void loop() {
  // read pot inputs -> duty -> PT4115 DIM outputs, one channel at a time
  for (uint8_t ch = 0; ch < pwm::CHANNEL_COUNT; ++ch) {
    smoothed[ch] = pwm::emaStep(smoothed[ch], analogRead(PIN_POT[ch]), EMA_SHIFT);
    uint8_t duty = pwm::dutyFromAdc(smoothed[ch]);
    analogWrite(PIN_PWM[ch], pwm::dutyToPwm8(duty));
  }
}
