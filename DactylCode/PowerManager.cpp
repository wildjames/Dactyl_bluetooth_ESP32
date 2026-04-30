#include "PowerManager.h"

#include <esp_sleep.h>

#include "HidDispatcher.h"
#include "MatrixScanner.h"
#include "StatusLed.h"

namespace PowerManager {

void update_battery_level(const BoardConfig& config, const LinkState& linkState, BatteryState& batteryState) {
  int battery_measurement = analogRead(config.battery.pin);
  float battery_voltage = battery_measurement * (config.battery.refVoltage * 2.0f * 1.1f / 4095.0f);
  float battery_percentage = 100.0f * (battery_voltage - config.battery.minVoltage)
                           / (config.battery.maxVoltage - config.battery.minVoltage);

  if (battery_percentage > 100.0f) { battery_percentage = 100.0f; }
  if (battery_percentage < 0.0f) { battery_percentage = 0.0f; }

  // The primary sends its battery level to the host. But, what about the secondary?
  // TODO: come up with a solution
  if (config.isPrimary) {
    HidDispatcher::set_battery_level(battery_percentage);
  }

  batteryState.lastUpdate = millis();
}

void enter_deep_sleep(const BoardConfig& config, LedState& ledState) {
  if (config.debug) { Serial.println("Entering deep sleep!"); }

  StatusLed::turn_off(config, ledState);

  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);

  uint64_t buttonPinMask = MatrixScanner::build_wake_pin_mask(config);
  if (config.debug) {
    Serial.print("Button pin mask is: ");
    Serial.println(buttonPinMask);
  }

  MatrixScanner::prepare_wake_pins(config);

  esp_sleep_enable_ext1_wakeup(buttonPinMask, ESP_EXT1_WAKEUP_ANY_HIGH);
  if (config.debug) { Serial.println("Going to sleep..."); }

  esp_deep_sleep_start();
}

}
