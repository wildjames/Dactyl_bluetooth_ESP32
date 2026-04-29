// ------------------------------//
// User-editable stuff goes here //
// ------------------------------//

#include "../BoardConfig.h"

// Keymaps
#include "KeyLayout_R.h"

// PINS!

// Software Serial pins
static const int SERIAL_RX_PIN = 16;
static const int SERIAL_TX_PIN = 17;

// Status LED
static const int LED_PIN = 21;

// Columns are READ
static const int COL_PINS[] = {27, 12, 13, 14, 15, 16, 17};

// Rows are DRIVEN HIGH
static const int ROW_PINS[] = {33, 32, 22, 23, 19};

// For waking from deepsleep,
// you *could* have these pins listen for a HIGH signal to wake the controller.
// Requires using RTC pins for rows: {0, 2, 4, 12-15, 25, 26, 27, 32-39}
// Selected wake columns:
// GPIO27 -> Backspace, Minus, Apostrophe, Right Shift
// GPIO13 -> 9, O, L, Period, Equals
// GPIO14 -> 8, I, K, Comma, Asterisk
static const int WAKE_PINS[] = {27, 13, 14};

HijelHID_BLEKeyboard bleKB("TwoBrownFoxes_R", "JWILD", 50);

inline BoardConfig make_board_config() {
  BoardConfig config = {};

  config.boardLabel = "Right half";
  config.bleDeviceName = "TwoBrownFoxes_R";
  config.primaryBleName = "TwoBrownFoxes";
  config.manufacturerName = "JWILD";
  config.debug = false;
  config.dummy = false;
  config.isPrimary = false;
  config.enableSerialSplit = false;
  config.serialRxPin = SERIAL_RX_PIN;
  config.serialTxPin = SERIAL_TX_PIN;
  config.colPins = COL_PINS;
  config.colCount = 7;
  config.rowPins = ROW_PINS;
  config.rowCount = 5;
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
  config.timings.disconnectedWaitMs = 500;
  config.timings.disconnectedDeepSleepMs = 1000 * 60 * 2;
  config.timings.keyDelayUs = 10;
  config.timings.deepSleepWaitMs = 1000 * 60 * 10;
  config.timings.batteryPollIntervalMs = 1000 * 10;
  config.timings.keepAliveDelayMs = 500;
  config.timings.keepAliveLifespanMs = 1500;

  return config;
}

const BoardConfig boardConfig = make_board_config();


// -------------------------------//
//   END OF USER-EDITABLE STUFF   //
// -------------------------------//
