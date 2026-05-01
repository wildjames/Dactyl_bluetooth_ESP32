// ------------------------------//
// User-editable stuff goes here //
// ------------------------------//

#include "../BoardConfig.h"

// Keymaps
#include "KeyLayout_R.h"

// PINS!

// Status LED (external, wired to D13 / GPIO 13)
static const int LED_PIN = 13;

// Columns are READ
// ESP32-S3 Feather: A0=GPIO18, A1=GPIO17, A2=GPIO16, A3=GPIO15, A4=GPIO14, A5=GPIO8, D12=GPIO12
static const int COL_PINS[] = {17, 12, 15, 16, 8, 14, 18};

// Rows are DRIVEN HIGH
// ESP32-S3 Feather: D5=GPIO5, D6=GPIO6, D9=GPIO9, D10=GPIO10, D11=GPIO11
static const int ROW_PINS[] = {10, 5, 6, 11, 9};

// For waking from deepsleep, all column pins are RTC-capable (GPIO 0-21)
// so we use all columns as wake pins.
static const int WAKE_PINS[] = {17, 12, 15, 16, 8, 14, 18};

HijelHID_BLEKeyboard bleKB("TwoBrownFoxes_R", "JWILD", 50);

inline BoardConfig make_board_config() {
  BoardConfig config = {};

  config.boardLabel = "Right half";
  config.bleDeviceName = "TwoBrownFoxes_R";
  config.primaryBleName = "TwoBrownFoxes";
  config.manufacturerName = "JWILD";
  config.debug = true;
  config.dummy = false;
  config.isPrimary = false;
  config.colPins = COL_PINS;
  config.colCount = 7;
  config.rowPins = ROW_PINS;
  config.rowCount = 5;
  config.wakePins = WAKE_PINS;
  config.wakeCount = 7;
  config.enableBatteryMonitoring = true;

  config.led.pin = LED_PIN;
  config.led.frequency = 5000;
  config.led.resolution = 8;
  config.led.maxDutyCycle = 200;

  config.timings.pollTimeMs = 5;
  config.timings.doubleTapIntervalMs = 1000;
  config.timings.doubleTapMinIntervalMs = 100;
  config.timings.disconnectedWaitMs = 500;
  config.timings.disconnectedDeepSleepMs = 1000 * 60 * 2;
  config.timings.keyDelayUs = 10;
  config.timings.deepSleepWaitMs = 1000 * 60 * 10;
  config.timings.batteryPollIntervalMs = 1000 * 10;

  return config;
}

const BoardConfig boardConfig = make_board_config();


// -------------------------------//
//   END OF USER-EDITABLE STUFF   //
// -------------------------------//
