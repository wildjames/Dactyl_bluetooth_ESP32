// DactylKeyFinder
// Scans the key matrix and prints the row and column of any pressed key over serial.
// No Bluetooth required. Select the correct pin configuration for your half below.

// ========== SELECT YOUR HALF ==========
// Uncomment ONE of the following:

#define RIGHT_HALF
// #define LEFT_HALF

// ========== PIN CONFIGURATION ==========

#ifdef RIGHT_HALF
// Right half: 5 rows x 7 cols
static const int ROW_PINS[] = {10, 5, 6, 11, 9};
static const int COL_PINS[] = {17, 12, 15, 16, 8, 14, 18};
static const int ROW_COUNT = 5;
static const int COL_COUNT = 7;
#endif

#ifdef LEFT_HALF
// Left half: 7 rows x 5 cols
static const int ROW_PINS[] = {5, 6, 8, 9, 10, 11, 12};
static const int COL_PINS[] = {18, 17, 16, 15, 14};
static const int ROW_COUNT = 7;
static const int COL_COUNT = 5;
#endif

// ========== STATE ==========

static bool keyStates[ROW_COUNT * COL_COUNT] = {false};
static bool previousKeyStates[ROW_COUNT * COL_COUNT] = {false};

// ========== SETUP ==========

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  Serial.println("DactylKeyFinder starting...");
  Serial.print("Rows: "); Serial.println(ROW_COUNT);
  Serial.print("Cols: "); Serial.println(COL_COUNT);
  Serial.println("Press any key to see its [row, col]");
  Serial.println("---");

  for (int i = 0; i < ROW_COUNT; i++) {
    pinMode(ROW_PINS[i], OUTPUT);
    digitalWrite(ROW_PINS[i], LOW);
  }

  for (int i = 0; i < COL_COUNT; i++) {
    pinMode(COL_PINS[i], INPUT_PULLDOWN);
  }
}

// ========== LOOP ==========

void loop() {
  int key_index = 0;

  for (int row = 0; row < ROW_COUNT; row++) {
    digitalWrite(ROW_PINS[row], HIGH);
    delayMicroseconds(10);

    for (int col = 0; col < COL_COUNT; col++) {
      bool value = digitalRead(COL_PINS[col]);

      previousKeyStates[key_index] = keyStates[key_index];
      keyStates[key_index] = value;

      // Print on key press (transition from LOW to HIGH)
      if (value && !previousKeyStates[key_index]) {
        int keyNumber = row * COL_COUNT + col;

        Serial.print("Key pressed:");
        Serial.print("  KeyIndex=");
        Serial.print(keyNumber);
        Serial.print("  row=");
        Serial.print(row);
        Serial.print("  col=");
        Serial.print(col);
        Serial.print("  (pin row=");
        Serial.print(ROW_PINS[row]);
        Serial.print(", pin col=");
        Serial.print(COL_PINS[col]);
        Serial.println(")");
      }

      key_index++;
    }

    digitalWrite(ROW_PINS[row], LOW);
  }

  delay(5);
}
