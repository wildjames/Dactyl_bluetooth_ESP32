// ------------------------------//
// User-editable stuff goes here //
// ------------------------------//

#include "BoardConfig.h"

// Keymaps
#include "KeyLayout_L.h"

// PINS!

// Software Serial pins
static const int SERIAL_RX_PIN = 16;
static const int SERIAL_TX_PIN = 17;

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

const BoardConfig boardConfig = {
	"Left half",
	"TwoBrownFoxes",
	"TwoBrownFoxes",
	"JWILD",
	false,
	false,
	true,
	SERIAL_RX_PIN,
	SERIAL_TX_PIN,
	COL_PINS,
	5,
	ROW_PINS,
	7,
	WAKE_PINS,
	3,
	{
		LED_PIN,
		5000,
		8,
		200,
	},
	{
		A13,
		3.3,
		3.2,
		4.2,
	},
	{
		5,
		1000,
		500,
		1000 * 60 * 2,
		10,
		1000 * 60 * 10,
		1000 * 10,
		500,
		1500,
	},
};

// -------------------------------//
//   END OF USER-EDITABLE STUFF   //
// -------------------------------//
