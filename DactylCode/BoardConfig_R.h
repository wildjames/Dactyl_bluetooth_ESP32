// ------------------------------//
// User-editable stuff goes here //
// ------------------------------//

// Keymaps
#include "KeyLayout_R.h"

// Software Serial pins
#define RXD2 16
#define TXD2 17

// Keys per half, NCOLS * NROWS, must be calculated by the user.
#define NKEYS 35


// In DEBUG mode, I send a load of info over Serial (USB). 
// If that's not necessary, why waste the cycles?
const bool DEBUG = false;
// In dummy mode, I don't actually send my key presses.
const bool DUMMY = false;


// Split keyboard?
const bool split_keeb_communication = false;
const bool is_master = false;


BleKeyboard bleKB("TwoBrownFoxes_R", "JWILD", 50);


// PINS!

// Status LED
const int LEDPin = 21;

// Columns are READ
int colPins[] = {27, 12, 13, 14, 15, 16, 17};
int NCOLS = 7;

// Rows are DRIVEN HIGH
int rowPins[] = {33, 32, 22, 23, 19};
int NROWS = 5;

// For waking from deepsleep, 
// you *could* have these pins listen for a HIGH signal to wake the controller.
// Requires using RTC pins for rows: {0, 2, 4, 12-15, 25, 26, 27, 32-39}
// CAUTION: UNTESTED AND UNUSED. SEE END OF MAIN CODE.
int wakePins[] = {};
int NWAKE = 0;

// For communicating the battery percentage
int batteryPin = A13;

float battery_ref_voltage = 3.3;
float battery_min_voltage = 3.2;
float battery_max_voltage = 4.2;


// LED brightness settings
int frequency = 5000;
const int ledChannel = 0;
const int resolution = 8;
const int max_duty_cycle = 200;

// TIMINGS!

// 125Hz is generally standard. ESP32 could push higher though. 
int poll_time = 5; // ms

// you can double-tap the modifier to lock it. 
int double_tap_interval = 1000; // ms

// When we're disconnected, we can sit on our hands for a while
int disconnected_wait = 500; // ms
// And if we're disconnected, we can have a lower threshold for going to sleep
int disconnected_deepsleep = 1000 * 60 * 2; // ms

// How long to wait between setting a row high, and reading the column voltage. 
// Pushing this too low induces phantom presses in adjacent keyswitch wires.
// 10us * 35 keys = 0.35ms. 
int key_delay_us = 10; // MICROseconds!

// how long after the last keystroke before entering deep sleep.
int deepsleep_wait = 1000 * 60 * 10; // ms

// How long between battery voltage updates. Will report garbage when charging
int battery_poll_interval = 1000 * 10; // ms

// Frequency of checking my connection to my partner
int keep_alive_delay = 500; //ms
int keep_alive_lifespan = 1500; //ms


// -------------------------------//
//   END OF USER-EDITABLE STUFF   //
// -------------------------------//
