#include "StatusLed.h"

namespace StatusLed {

void begin(const BoardConfig& config, LedState& ledState) {
  ledcAttach(config.led.pin, config.led.frequency, config.led.resolution);
  ledState.dutyCycle = config.led.maxDutyCycle;
}

void show_connected(const BoardConfig& config, LedState& ledState, const KeymapResolver::KeyboardState& keyboardState) {
  ledState.outputState = HIGH;
  if (keyboardState.lockedModKey) {
    if (millis() - ledState.lastFlashToggle >= 125) {
      ledState.flashHigh = !ledState.flashHigh;
      ledState.lastFlashToggle = millis();
    }

    ledState.dutyCycle = ledState.flashHigh
      ? config.led.maxDutyCycle
      : config.led.maxDutyCycle / 2;
  } else {
    ledState.dutyCycle = config.led.maxDutyCycle / 2;
  }

  ledcWrite(config.led.pin, ledState.outputState * ledState.dutyCycle);
}

void show_disconnected(const BoardConfig& config, LedState& ledState) {
  ledState.outputState = ledState.outputState == HIGH ? LOW : HIGH;
  ledcWrite(config.led.pin, ledState.outputState * config.led.maxDutyCycle);
}

void turn_off(const BoardConfig& config, LedState& ledState) {
  ledState.outputState = LOW;
  digitalWrite(config.led.pin, ledState.outputState);
}

}
