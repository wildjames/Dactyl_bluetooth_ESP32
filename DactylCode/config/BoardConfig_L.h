// ------------------------------//
// User-editable stuff goes here //
// ------------------------------//

#include "../BoardConfig.h"

// Keymaps
#include "KeyLayout_L.h"

// PINS!

// Status LED (external, wired to D13 / GPIO 13)
static const int LED_PIN = 13;

static const int COL_PINS[] = {11, 10, 9, 5, 6};
static const int ROW_PINS[] = {14, 15, 12, 16, 18, 8, 17};

// For waking from deepsleep, all column pins are RTC-capable (GPIO 0-21)
// so we use all columns as wake pins.
static const int WAKE_PINS[] = {11, 10, 9, 5, 6};

// TODO: This should live in the HidDispatcher. Note it is only referenced there and in powermanager, so HidDispatcher will need a beforeSleep()
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
  config.wakeCount = 5;
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
