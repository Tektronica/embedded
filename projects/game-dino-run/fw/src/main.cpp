#include <Arduino.h>
#include <TM1637Display.h>
#include <U8glib.h>

#include "Buzzer.h"
#include "Dino.h"
#include "GameState.h"
#include "Obstacles.h"
#include "Score.h"
#include "Sprites.h"

namespace {

// Rotary encoder module: only its pushbutton (SW) is used, via interrupt. CLK/DT (rotation) are
// wired for a future use but never read -- left configured as inputs so the physical module has
// defined pin states, matching the diagram, but there's no rotation-handling code to speak of.
constexpr uint8_t PIN_ENCODER_CLK = 2;
constexpr uint8_t PIN_ENCODER_SW = 3;
constexpr uint8_t PIN_ENCODER_DT = 4;

constexpr uint8_t PIN_SCORE_DIO = 5;
constexpr uint8_t PIN_SCORE_CLK = 6;
constexpr uint8_t PIN_BUZZER = 10;
// OLED panel is I2C (A4/A5 on the Nano); U8glib addresses it internally, no pin constants needed.

constexpr uint32_t BUTTON_DEBOUNCE_MS = 180;
constexpr uint8_t  OBSTACLE_SPEED = 8;
constexpr uint8_t  SCORE_DISPLAY_BRIGHTNESS = 7;
constexpr int16_t  CLOUD_WRAP_X = -38;
constexpr int16_t  CLOUD_RESET_X = 128;
constexpr int16_t  OBSTACLE_GAP_MIN = 80;
constexpr int16_t  OBSTACLE_GAP_MAX = 125;  // random()'s upper bound is exclusive

TM1637Display     scoreDisplay(PIN_SCORE_CLK, PIN_SCORE_DIO);
U8GLIB_SH1106_128X64 panel(U8G_I2C_OPT_NONE);

gamestate::State  state = gamestate::State::Start;
dino::State       dinoState;
obstacles::State  obstacleState;
int16_t           cloudX = CLOUD_RESET_X;
uint32_t          startMs = 0;
uint16_t          lastMilestone = 0;

buzzer::Pattern activePattern = buzzer::Pattern::None;
uint32_t        patternStartMs = 0;

// Set only by the ISR below; processed (and cleared) once per loop() -- keeps the ISR itself
// down to a debounce check and a flag set, no game logic or peripheral calls inside it.
volatile bool buttonPressedFlag = false;

void startBuzzerPattern(buzzer::Pattern pattern) {
  activePattern = pattern;
  patternStartMs = millis();
}

// Renders the active pattern's tone state to the buzzer pin every loop -- no delay(), so a
// milestone or hit sound never freezes animation/input the way the original's blocking two-tone
// sequences did.
void updateBuzzer() {
  uint32_t elapsedMs = millis() - patternStartMs;
  if (buzzer::isFinished(activePattern, elapsedMs)) activePattern = buzzer::Pattern::None;

  buzzer::ToneState toneState = buzzer::toneStateFor(activePattern, elapsedMs);
  if (toneState.on) tone(PIN_BUZZER, toneState.frequencyHz);
  else noTone(PIN_BUZZER);
}

void resetGame() {
  dinoState = dino::State{};
  obstacleState = obstacles::State{};
  cloudX = CLOUD_RESET_X;
  startMs = millis();
  lastMilestone = 0;
}

// Interrupt handler for the encoder's pushbutton: debounce by timestamp (cheap, no peripheral
// calls), then just set a flag. Deliberately does not call tone() or touch game state directly --
// the original sketch did both from inside the ISR, which is more work than an ISR should do.
void onButtonPressed() {
  static uint32_t lastInterruptMs = 0;
  uint32_t nowMs = millis();
  if (nowMs - lastInterruptMs > BUTTON_DEBOUNCE_MS) buttonPressedFlag = true;
  lastInterruptMs = nowMs;
}

// A press means different things depending on game state: while Playing it's a jump; otherwise
// it's the Start/End state transition (see GameState.h).
void handleButtonPress() {
  if (state == gamestate::State::Playing) {
    if (dino::startJump(dinoState)) startBuzzerPattern(buzzer::Pattern::Jump);
    return;
  }
  state = gamestate::next(state);
  if (state == gamestate::State::Playing) resetGame();
}

void updateScore() {
  if (state != gamestate::State::Playing) return;

  uint16_t currentScore = score::compute(millis() - startMs, OBSTACLE_SPEED);
  scoreDisplay.showNumberDecEx(currentScore);

  uint16_t newMilestone = score::milestoneFor(currentScore, lastMilestone);
  if (newMilestone > lastMilestone) {
    lastMilestone = newMilestone;
    startBuzzerPattern(buzzer::Pattern::Milestone);
  }
}

void u8gPrepare() {
  panel.setFont(u8g_font_6x10);
  panel.setFontRefHeightExtendedText();
  panel.setDefaultForegroundColor();
  panel.setFontPosTop();
}

void drawDino() {
  if (state == gamestate::State::End) {
    panel.drawXBMP(0, 43 - dinoState.jumpHeight, 20, 21, sprites::dinoBlah);
    return;
  }
  if (dinoState.phase != dino::JumpPhase::Grounded) {
    panel.drawXBMP(0, 43 - dinoState.jumpHeight, 20, 21, sprites::dinoJump);
    return;
  }
  switch (dinoState.legFrame) {
    case 0: panel.drawXBMP(0, 43, 20, 21, sprites::dinoJump); break;
    case 1: panel.drawXBMP(0, 43, 20, 21, sprites::dinoLeft); break;
    case 2: panel.drawXBMP(0, 43, 20, 21, sprites::dinoRight); break;
  }
}

void drawShape(obstacles::Shape shape, int16_t x) {
  switch (shape) {
    case obstacles::Shape::OneTall:    panel.drawXBMP(x, 44, 10, 20, sprites::oneCactus); break;
    case obstacles::Shape::TwoTall:    panel.drawXBMP(x, 44, 20, 20, sprites::twoCactus); break;
    case obstacles::Shape::ThreeTall:  panel.drawXBMP(x, 44, 20, 20, sprites::threeCactus); break;
    case obstacles::Shape::OneSmall:   panel.drawXBMP(x, 52, 6, 12, sprites::oneCactusSmall); break;
    case obstacles::Shape::TwoSmall:   panel.drawXBMP(x, 52, 12, 12, sprites::twoCactusSmall); break;
    case obstacles::Shape::ThreeSmall: panel.drawXBMP(x, 52, 17, 12, sprites::threeCactusSmall); break;
  }
}

void drawObstacles() {
  drawShape(obstacleState.front.shape, obstacleState.front.x);
  drawShape(obstacleState.next.shape, obstacleState.next.x);
}

void draw() {
  u8gPrepare();
  switch (state) {
    case gamestate::State::Start:
      panel.drawStr(0, 10, "Welcome to");
      panel.drawStr(10, 30, "Dino!!");
      panel.drawStr(0, 50, "Push to begin");
      break;

    case gamestate::State::Playing:
      drawDino();
      panel.drawXBMP(cloudX, 5, 39, 12, sprites::cloud);
      drawObstacles();
      break;

    case gamestate::State::End:
      panel.drawXBMP(14, 12, 100, 15, sprites::gameOver);
      drawDino();
      panel.drawXBMP(cloudX, 5, 39, 12, sprites::cloud);
      drawObstacles();
      break;
  }
}

}  // namespace

void setup() {
  scoreDisplay.setBrightness(SCORE_DISPLAY_BRIGHTNESS);
  scoreDisplay.clear();

  pinMode(PIN_ENCODER_CLK, INPUT);
  pinMode(PIN_ENCODER_DT, INPUT);
  pinMode(PIN_ENCODER_SW, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_SW), onButtonPressed, FALLING);
}

void loop() {
  if (buttonPressedFlag) {
    buttonPressedFlag = false;
    handleButtonPress();
  }

  updateScore();

  panel.firstPage();
  do {
    draw();
  } while (panel.nextPage());

  if (state == gamestate::State::Playing) {
    dino::advance(dinoState);

    cloudX = (cloudX < CLOUD_WRAP_X) ? CLOUD_RESET_X : cloudX - 1;

    obstacles::advance(obstacleState, OBSTACLE_SPEED,
                        static_cast<int16_t>(random(OBSTACLE_GAP_MIN, OBSTACLE_GAP_MAX)),
                        static_cast<obstacles::Shape>(random(1, 7)));

    if (obstacles::collides(obstacleState.front.shape, obstacleState.front.x,
                             dinoState.jumpHeight)) {
      state = gamestate::State::End;
      startBuzzerPattern(buzzer::Pattern::Hit);
    }
  }

  updateBuzzer();
}
