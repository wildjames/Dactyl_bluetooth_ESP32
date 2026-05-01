#include "PowerManager.h"

#include <Adafruit_MAX1704X.h>
#include <Wire.h>
#include <esp_sleep.h>

#include "GattRelay.h"
#include "HidDispatcher.h"
#include "MatrixScanner.h"
#include "StatusLed.h"

namespace {

Adafruit_MAX17048 batteryMonitor;
bool batteryMonitorInitialized = false;
bool batteryMonitorAvailable = false;

void mark_battery_unavailable(BatteryState& batteryState) {
  batteryState.monitorAvailable = false;
  batteryState.voltage = NAN;
  batteryState.percentage = NAN;
  batteryState.chargeRate = NAN;
  batteryState.lastUpdate = millis();
}

void enable_i2c_power() {
#if defined(PIN_I2C_POWER)
  pinMode(PIN_I2C_POWER, OUTPUT);
  digitalWrite(PIN_I2C_POWER, HIGH);
#elif defined(I2C_POWER)
  pinMode(I2C_POWER, OUTPUT);
  digitalWrite(I2C_POWER, HIGH);
#endif
}

}

namespace PowerManager {

void begin(const BoardConfig& config, BatteryState& batteryState) {
  enable_i2c_power();

  if (batteryMonitorInitialized) {
    batteryState.monitorAvailable = batteryMonitorAvailable;
    return;
  }

  Wire.begin();
  batteryMonitorAvailable = batteryMonitor.begin(&Wire);
  batteryMonitorInitialized = true;

  if (!batteryMonitorAvailable) {
    if (config.debug) {
      Serial.println("Battery monitor not found");
    }
    mark_battery_unavailable(batteryState);
    return;
  }

  batteryState.monitorAvailable = true;
  if (config.debug) {
    Serial.print("Battery monitor ready, chip ID: 0x");
    Serial.println(batteryMonitor.getChipID(), HEX);
  }
}

void update_battery_level(const BoardConfig& config, const LinkState& linkState, BatteryState& batteryState) {
  if (!batteryMonitorInitialized) {
    begin(config, batteryState);
  }

  if (!batteryMonitorAvailable) {
    mark_battery_unavailable(batteryState);
    return;
  }

  float battery_voltage = batteryMonitor.cellVoltage();
  float battery_percentage = batteryMonitor.cellPercent();
  float battery_charge_rate = batteryMonitor.chargeRate();

  if (isnan(battery_voltage) || isnan(battery_percentage)) {
    mark_battery_unavailable(batteryState);
    return;
  }

  if (battery_percentage > 100.0f) { battery_percentage = 100.0f; }
  if (battery_percentage < 0.0f) { battery_percentage = 0.0f; }

  batteryState.monitorAvailable = true;
  batteryState.voltage = battery_voltage;
  batteryState.percentage = battery_percentage;
  batteryState.chargeRate = battery_charge_rate;

  if (config.isPrimary) {
    // Report the minimum of both halves so Windows shows the worst-case level.
    float reportedLevel = battery_percentage;
    if (!isnan(batteryState.companionPercentage) && batteryState.companionPercentage < reportedLevel) {
      reportedLevel = batteryState.companionPercentage;
    }
    HidDispatcher::set_battery_level(reportedLevel);

    uint8_t primaryPct = (uint8_t)battery_percentage;
    uint8_t companionPct = isnan(batteryState.companionPercentage)
                             ? 0xFF
                             : (uint8_t)batteryState.companionPercentage;
    update_battery_scan_response(primaryPct, companionPct);
  } else if (linkState.isConnected) {
    gatt_send_battery_level((uint8_t)battery_percentage);
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
