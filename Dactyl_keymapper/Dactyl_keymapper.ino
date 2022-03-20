#define USE_NIMBLE
#include <BleKeyboard.h>

// ------------------------------//
// User-editable stuff goes here //
// ------------------------------//

int colPins[] = {12, 33, 27, 25, 13, 14, 26};
int NCOLS = 7;

int rowPins[] = {4, 5, 2, 18, 15};
int NROWS = 5;

// Keys per half, NCOLS * NROWS
#define NKEYS 35


BleKeyboard bleKB("Keymapper");

// -------------------------------//
//   END OF USER-EDITABLE STUFF   //
// -------------------------------//

int keyStates[NKEYS] = {0};
int pKeyStates[NKEYS] = {0};

int last_loop;
int poll_time = 8;
int key_delay_us = 10; // 8ms / 50 keys = 160us


void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(1000);
  Serial.println("BOOTED");

  Serial.println("Row nums:");
  for (int i=0; i<NROWS; i++) {
    Serial.println(rowPins[i]);
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], LOW);
  }
  Serial.println("Col nums:");
  for (int i=0; i<NCOLS; i++) {
    Serial.println(colPins[i]);
    pinMode(colPins[i], INPUT_PULLDOWN);
  }

  bleKB.begin();
  
  last_loop = millis();

  get_keymap();
}


void loop() {
  get_keymap();
}


void get_keymap() {
  int m = 0;
  char letters[] = {
    'a', 'b', 'c', 'd', 'e',
    'f', 'g', 'h', 'i', 'j',
    'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y',
    'z', ' ',
    '1', '2', '3', '4', '5',
    '6', '7', '8', '9', '0'
  };

  char keyMap[NKEYS] = {' '};

  bool cont = true;

  for (int n=0; n<37; n++) {
    Serial.print("Hit the key for: ");
    Serial.println(letters[n]);

    cont = true;
    while (cont) {
      if (Serial.available()) {
        Serial.println("Skipping character!");
        cont = false;
        while (Serial.available()) {Serial.read();}
      } else {
        int k = 0;
        for (int i=0; i<NROWS; i++) {
          digitalWrite(rowPins[i], HIGH);
          for (int j=0; j<NCOLS; j++) {
            bool val = digitalRead(colPins[j]);
            k++;
      
            // Store the current and previous key states
            pKeyStates[k] = keyStates[k];
            keyStates[k] = val;
            
            if (keyStates[k] and (not pKeyStates[k])) {
              Serial.print("PRESSED: ");
              Serial.print(rowPins[i]);
              Serial.print(" - ");
              Serial.print(colPins[j]);
              Serial.print(" | AKA key ");
              Serial.println(k);
  
              keyMap[k] = letters[n];
              cont = false;

            // now wait for the key to be released
            while (digitalRead(colPins[j]));
            }
          }
          digitalWrite(rowPins[i], LOW);
        }
      }
    }
  }

  Serial.println("OK DONE");
  Serial.println("Keymap:");
  Serial.print("{'");
  for (int i=0; i<NKEYS; i++) {
    Serial.print(keyMap[i]);
    Serial.print("', '");
  }
  Serial.print("};");
}


void poll_pins() {
  // Counter for what key we're looking at
  int k = 0;
  
  for (int i=0; i<NROWS; i++) {
    digitalWrite(rowPins[i], HIGH);
    for (int j=0; j<NCOLS; j++) {
      bool val = digitalRead(colPins[j]);
      k++;

      // Store the current and previous key states
      pKeyStates[k] = keyStates[k];
      keyStates[k] = val;
      
      if (keyStates[k] and (not pKeyStates[k])) {
        Serial.print("PRESSED: ");
        Serial.print(rowPins[i]);
        Serial.print(" - ");
        Serial.print(colPins[j]);
        Serial.print(" | AKA key ");
        Serial.println(k);
      }
      if ((not keyStates[k]) and pKeyStates[k]) {
        Serial.print("RELEASED: ");
        Serial.print(rowPins[i]);
        Serial.print(" - ");
        Serial.print(colPins[j]);
        Serial.print(" | AKA key ");
        Serial.println(k);
      }
    }
    digitalWrite(rowPins[i], LOW);
    delayMicroseconds(key_delay_us);
  }
}
