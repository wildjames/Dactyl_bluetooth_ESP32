#include "StatusLed.h"

namespace StatusLed {

void begin(const BoardConfig& config, LedState& ledState) {
  ledcAttach(config.led.pin, config.led.frequency, config.led.resolution);
  ledState.dutyCycle = config.led.maxDutyCycle;
}

void update(const BoardConfig& config, LedState& ledState) {
  switch (ledState.mode) {
    case LedMode::Off:
      ledState.outputState = LOW;
      ledState.dutyCycle = 0;
      break;

    case LedMode::Disconnected:
      if (millis() - ledState.lastFlashToggle >= (unsigned long)config.timings.disconnectedWaitMs) {
        ledState.outputState = ledState.outputState == HIGH ? LOW : HIGH;
        ledState.lastFlashToggle = millis();
      }
      ledState.dutyCycle = config.led.maxDutyCycle;
      break;

    case LedMode::Connected:
      ledState.outputState = HIGH;
      ledState.dutyCycle = config.led.maxDutyCycle / 2;
      break;

    case LedMode::ConnectedModLocked:
      ledState.outputState = HIGH;
      if (millis() - ledState.lastFlashToggle >= 125) {
        ledState.flashHigh = !ledState.flashHigh;
        ledState.lastFlashToggle = millis();
      }
      ledState.dutyCycle = ledState.flashHigh
        ? config.led.maxDutyCycle
        : config.led.maxDutyCycle / 2;
      break;
  }

  ledcWrite(config.led.pin, ledState.outputState * ledState.dutyCycle);
}

}
