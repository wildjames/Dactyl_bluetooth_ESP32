// ------------------------------//
// User-editable stuff goes here //
// ------------------------------//

#include "../BoardConfig.h"

// Keymaps
#include "KeyLayout_L.h"

// PINS!

// Status LED
static const int LED_PIN = 21;

// Columns are READ
static const int COL_PINS[] = {33, 12, 13, 14, 15};

// Rows are DRIVEN HIGH
static const int ROW_PINS[] = {27, 32, 22, 23, 19, 16, 17};

// For waking from deepsleep,
// you *could* have these pins listen for a HIGH signal to wake the controller.
// Requires using RTC pins for rows: {0, 2, 4, 12-15, 25, 26, 27, 32-39}
// Selected wake columns:
// GPIO33 -> Esc, 1, 2, 3, 4, 5, Home
// GPIO13 -> Left Shift, A, S, D, F, G
// GPIO14 -> Left Ctrl, Z, X, C, V, B, Space
static const int WAKE_PINS[] = {33, 13, 14};

HijelHID_BLEKeyboard bleKB("TwoBrownFoxes", "JWILD", 50);

inline BoardConfig make_board_config() {
  BoardConfig config = {};

  config.boardLabel = "Left half";
  config.bleDeviceName = "TwoBrownFoxes";
  config.primaryBleName = "TwoBrownFoxes";
  config.manufacturerName = "JWILD";
  config.debug = false;
  config.dummy = false;
  config.isPrimary = true;
  config.colPins = COL_PINS;
  config.colCount = 5;
  config.rowPins = ROW_PINS;
  config.rowCount = 7;
  config.wakePins = WAKE_PINS;
  config.wakeCount = 3;

  config.led.pin = LED_PIN;
  config.led.frequency = 5000;
  config.led.resolution = 8;
  config.led.maxDutyCycle = 200;

  config.battery.pin = A13;
  config.battery.refVoltage = 3.3;
  config.battery.minVoltage = 3.2;
  config.battery.maxVoltage = 4.2;

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
