// TODO:
// [DONE] recognise keypresses and releases consistently
// [DONE] make Bluetooth keyboard work
// [DONE] Hook keypresses into bluetooth keyboard
// [DONE] Make a keymap
// [DONE] impliment layers to the keyboard
// [DONE] communication between two ESP32 down the jack
// [DONE] make the left keyboard pass on keystrokes from the right one
// [DONE] The two halves need to communicate when they are plugged
//   - send a keep-alive message every second or so, if it's missed we're no longer
//     connected.~
//   - Code for this is implemented, but I don't really know what to do between the boards.
//     Might have been better as a way to share power delivery? dunno. Since layers are localised
//     to each board anyway, the modifier states don't need to be shared. Unsure if there
//     might be a latency improvement if only one BT device is sending keys? not sure how to test. I doubt it would be significant.
// [DONE] report battery level of the keyboard halves
// The layout needs optimising.
// [DONE] Remove game mode, I dont need it.
//   - I made it an optional toggle in the config files
// [DONE]Rewire the connecting cable so that the two halves can share a charging current.
//   - Check if the cable is thick enough to carry 500mA (YES)
//   - Make this so that it connects the *chargers*, not the *batteries*. If two batteries
//     of different voltage are connected, they'll equalise very quickly and probably saturate their max current
//     which could start a fire!
//   - The charging boards to provide input pads - could likely solder them together?
// [DONE] dim leds with some strobing
// [DONE] Add  double tap to lock the modifier
// [DONE] double tap to lock shift to caps?
// [DONE] light sleep between loops, if theres time

// [TODO] Detect if we are plugged in to another half, and dynamically switch to the connected mode


#include <HijelHID_BLEKeyboard.h>


//******************************************************************

// board-specific info in a header file. Make sure to change this!
// #include "BoardConfig_L.h"
#include "BoardConfig_R.h"

// Wireless GATT relay between halves (must come after BoardConfig so that
// bleKB, DEBUG, is_primary, RXD2/TXD2 are already declared).
#include "GattRelay.h"

//******************************************************************


// For keeping GPIO high during deep sleep
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"

// Tracks the keys.
int keyStates[NKEYS] = { 0 };
int pKeyStates[NKEYS] = { 0 };

// Connection-mode flags – set once at boot by detect_wired_connection().
// split_keeb_communication: true  → wired Serial2 path (original behaviour).
// use_gatt:                 true  → wireless GATT path (new behaviour).
bool split_keeb_communication = false;
bool use_gatt = false;

// For communicating with the other half - sends this message first
// to denote a press or release
int press_flag = B00001111;
int release_flag = B11110000;

// This is some placeholder code, for now establishing connection and disconnection.
// I don't know what the benefits would be for communicating, since my layers are localised
// to each half... Left in anyway, for possible future work.
bool is_connected = false;
int keep_alive = 50;
int last_keep_alive_check;
int last_keep_alive_time = 0;
uint8_t last_gatt_connected_count = 0;

// Battery monitor timing
int last_battery_update = 0;

// Double-tappable modifier key
int last_mod_tap = 0;
bool locked_modkey = false;
int last_mod_flash = 0;
bool mod_flash_high = false;

// Double-tappable shift key (sends caps lock)
int last_shift_tap = 0;

// For seeing if this loop is done yet.
int last_loop;

// Use an alternate keymap
bool is_alt = false;

// Time of the last keypress
int last_keypress;

// Current LED state.
int led_state = HIGH;
int duty_cycle;


void release_sleep_matrix_config() {
  gpio_deep_sleep_hold_dis();

  for (int i = 0; i < NROWS; i++) {
    gpio_hold_dis((gpio_num_t)rowPins[i]);
    if (rtc_gpio_is_valid_gpio((gpio_num_t)rowPins[i])) {
      rtc_gpio_deinit((gpio_num_t)rowPins[i]);
    }
  }

  for (int i = 0; i < NWAKE; i++) {
    gpio_hold_dis((gpio_num_t)wakePins[i]);
    if (rtc_gpio_is_valid_gpio((gpio_num_t)wakePins[i])) {
      rtc_gpio_deinit((gpio_num_t)wakePins[i]);
    }
  }
}


void setup() {
  release_sleep_matrix_config();

  if (DEBUG) {
    Serial.begin(115200);
    delay(1000);
    Serial.print("BOOTED ");
    if (is_primary) {
      Serial.println("PRIMARY");
    } else {
      Serial.println("SECONDARY");
    }
  }

  // ── Connection-mode detection (must run before bleKB.begin) ──────────────
  // detect_wired_connection() opens Serial2 and exchanges a SYNC/ACK handshake.
  // If the other half replies within 2 s we are wired; otherwise go wireless.
  if (detect_wired_connection()) {
    split_keeb_communication = true;
    use_gatt = false;
    if (DEBUG) { Serial.println("Connection: WIRED (Serial2)"); }
  } else {
    split_keeb_communication = false;
    use_gatt = true;
    if (DEBUG) { Serial.println("Connection: WIRELESS (GATT)"); }
  }

  if (DEBUG) { Serial.println("Row nums:"); }
  for (int i = 0; i < NROWS; i++) {
    if (DEBUG) { Serial.println(rowPins[i]); }
    pinMode(rowPins[i], OUTPUT);
    gpio_hold_dis((gpio_num_t)rowPins[i]);
    digitalWrite(rowPins[i], LOW);
  }
  if (DEBUG) { Serial.println("Col nums:"); }
  for (int i = 0; i < NCOLS; i++) {
    if (DEBUG) { Serial.println(colPins[i]); }
    pinMode(colPins[i], INPUT_PULLDOWN);
  }

  // Set up the LED PWM handler
  ledcAttach(LEDPin, frequency, resolution);
  duty_cycle = max_duty_cycle;

  // ── BLE / GATT initialisation ─────────────────────────────────────────────
  // Wireless secondary half skips bleKB (no HID advertising needed); it uses
  // NimBLE as a central in connect_to_primary_gatt() instead.
  if (!(use_gatt && !is_primary)) {
    bleKB.begin();
  }

  if (use_gatt) {
    if (is_primary) {
      // Add the relay GATT service on top of the existing HID server.
      setup_gatt_server();
    } else {
      // Scan for and connect to the primary half's relay service.
      // is_connected is set to true inside connect_to_primary_gatt() on success.
      connect_to_primary_gatt();
    }
  }

  last_mod_tap = millis();
  last_keep_alive_check = millis();
  last_keypress = millis();

  update_battery_level();

  last_loop = millis();
}


void loop() {
  bool primary_has_ble_peer = false;
  bool is_wireless_secondary = use_gatt && !is_primary;

  if (is_wireless_secondary && !is_connected) {
    connect_to_primary_gatt();
  }

  if (use_gatt && is_primary) {
    NimBLEServer* server = NimBLEDevice::getServer();
    if (server != nullptr) {
      uint8_t connected_count = server->getConnectedCount();
      primary_has_ble_peer = connected_count > 0;
      NimBLEAdvertising* advertising = server->getAdvertising();
      if (advertising != nullptr
          && connected_count > 0
          && connected_count != last_gatt_connected_count
          && !advertising->isAdvertising()) {
        server->startAdvertising();
      }
      last_gatt_connected_count = connected_count;
    }
  }

  bool has_primary_ble_link = bleKB.isConnected() || bleKB.isPaired();
  bool can_primary_send_keys = has_primary_ble_link || primary_has_ble_peer;

  // Wireless secondary half uses its relay link state. Every other mode should stay
  // active for any live BLE link, even if the HID library's paired/authenticated
  // bit gets cleared by a non-host reconnect on the shared NimBLE server.
  bool keyboard_active = is_wireless_secondary ? is_connected
                                               : (can_primary_send_keys || is_connected);

  if (keyboard_active) {
    // See if I have any changes to my pins
    poll_pins();
    // Handle and keypresses on my side of things
    parse_keypress();
    // See what my other half is doing
    if (split_keeb_communication) { parse_other_half(); }

    // Set the LED brightness.
    led_state = HIGH;
    if (locked_modkey) {
      if (millis() - last_mod_flash >= 125) {
        mod_flash_high = !mod_flash_high;
        last_mod_flash = millis();
      }
      duty_cycle = mod_flash_high ? max_duty_cycle : max_duty_cycle / 2;
    } else {
      duty_cycle = max_duty_cycle / 2;
    }
    ledcWrite(LEDPin, led_state * duty_cycle);

    if (split_keeb_communication) {
      if (millis() - last_keep_alive_check > keep_alive_delay) {
        send_keep_alive();
      }
      if (millis() - keep_alive_lifespan > last_keep_alive_time) {
        is_connected = false;
      }
    }

    // Yield for the remainder of the poll window
    int remaining_ms = poll_time - (int)(millis() - last_loop);
    if (remaining_ms > 1) {
      delay(remaining_ms);
    }

  } else {
    if (DEBUG) { Serial.println("Not connected to bluetooth..."); }

    // Flash the LED
    if (led_state == HIGH) {
      led_state = LOW;
    } else {
      led_state = HIGH;
    }
    ledcWrite(LEDPin, led_state * max_duty_cycle);

    // See what my other half is doing
    if (split_keeb_communication) { parse_other_half(); }

    // Yield for the disconnected wait window
    int remaining_ms = disconnected_wait - (int)(millis() - last_loop);
    if (remaining_ms > 1) {
      delay(remaining_ms);
    }

    // Unless it's been a while. Then give up and go to sleep
    if (millis() - last_keypress > disconnected_deepsleep) {
      go_to_sleep();
    }
  }

  // Do I need to update the battery?
  if (millis() - last_battery_update > battery_poll_interval) {
    update_battery_level();
  }

  // Do I want to go to sleep?
  if (millis() - last_keypress > deepsleep_wait) {
    go_to_sleep();
  }

  last_loop = millis();
}


void update_battery_level() {
  int battery_measurement = analogRead(batteryPin);
  float battery_voltage = battery_measurement * (battery_ref_voltage * 2.0 * 1.1 / 4095);
  // min voltage is 3.2, max is 4.2
  float battery_percentage = 100.0 * (battery_voltage - battery_min_voltage) / (battery_max_voltage - battery_min_voltage);

  if (battery_percentage > 100.0) { battery_percentage = 100.0; }
  if (battery_percentage < 0.0) { battery_percentage = 0.0; }

  //  if (DEBUG) {
  //    Serial.print("My battery pin measured ");
  //    Serial.println(battery_measurement);
  //    Serial.print("Which corresponds to a voltage of ");
  //    Serial.println(battery_voltage);
  //    Serial.print("And this is ");
  //    Serial.print(battery_percentage);
  //    Serial.println("% full.");
  //  }

  // Wireless secondary half has no HID connection, so there's nowhere to report battery.
  if (!(use_gatt && !is_primary)) {
    bleKB.setBatteryLevel(battery_percentage);
  }
  last_battery_update = millis();
}


void poll_pins() {
  // Counter for what key we're looking at
  int k = 0;

  // Loop over and check what the current state is. Store previous state as well.
  for (int i = 0; i < NROWS; i++) {
    digitalWrite(rowPins[i], HIGH);
    for (int j = 0; j < NCOLS; j++) {
      bool val = digitalRead(colPins[j]);

      // Store the current and previous key states
      pKeyStates[k] = keyStates[k];
      keyStates[k] = val;

      k++;
    }
    digitalWrite(rowPins[i], LOW);
    delayMicroseconds(key_delay_us);
  }


  // Remember what time we last pressed the modifier key
  if (pKeyStates[MODKEY0] and (not keyStates[MODKEY0])) {
    if (DEBUG) {
      Serial.println("Pressed the modifier key!");
      Serial.print("Time since last tap: ");
      Serial.println(millis() - last_mod_tap);
    }
    // But first check if we got a double tap.
    if ((millis() - last_mod_tap) < double_tap_interval) {
      if (DEBUG) {
        Serial.println("Toggling modifier lock");
      }
      locked_modkey = !locked_modkey;
      // Set the last tap to be somewhat in the past, so the next hit doesnt also trigger it
      last_mod_tap = last_mod_tap - double_tap_interval;
    } else {
      // Then, save the current tap time.
      last_mod_tap = millis();
    }
  }

  // Double-tap the shift key to toggle caps lock
  if (SHIFTKEY0 >= 0 && pKeyStates[SHIFTKEY0] && (not keyStates[SHIFTKEY0])) {
    if ((millis() - last_shift_tap) < double_tap_interval) {
      if (DEBUG) { Serial.println("Toggling caps lock"); }
      bleKB.tap(KEY_CAPS_LOCK);
      last_shift_tap = last_shift_tap - double_tap_interval;
    } else {
      last_shift_tap = millis();
    }
  }
}


void parse_keypress() {
  if (is_alt) {
    parse_alt();
  } else {
    parse_typing();
  }
}

void parse_alt() {
  // Handle toggling back to typing
  if (typing_toggle >= 0) {
    if (keyStates[typing_toggle]) {
      Serial.println("\nSWAPPING TO TYPING MODE\n");
      is_alt = false;

      // release all keys
      bleKB.releaseAll();
      // To prevent the toggle key firing its key on the new layer, set its pState to True
      pKeyStates[typing_toggle] = 1;

      return;
    }
  }

  send_keypress(alt_keymap);
}

void parse_typing() {
  if (alt_toggle >= 0) {
    if (keyStates[alt_toggle]) {
      Serial.println("\nSWAPPING TO alt MODE\n");
      is_alt = true;

      // release all keys
      bleKB.releaseAll();
      return;
    }
  }

  send_keypress(keymap);
}


void send_keypress(int keys[]) {
  int pressed = 0;
  // Handle layering
  if (keyStates[MODKEY0] or locked_modkey) {
    pressed += NKEYS;
  }

  for (int i = 0; i < NKEYS; i++) {
    if (keyStates[i] and (not pKeyStates[i])) {
      pressed += i;
      int letterIndex = keys[pressed];

      // Skip NC keys
      if (letterIndex == -1) {
        if (DEBUG) {
          Serial.println("Got a no-connect key");
        }
      } else if (letterIndex < -1) {
        // Handling Media keys
        letterIndex *= -1;  // Make it positive

        if (is_primary || (!use_gatt && !is_connected)) {
          if (not DUMMY) {
            bleKB.tap(media_keys[letterIndex]);
          }
        } else if (use_gatt) {
          gatt_send_media_key(media_keys[letterIndex]);
        }

        if (DEBUG) {
          Serial.print("Detected a keypress at Media key, index: ");
          Serial.println(letterIndex);
        }
      } else {
        last_keypress = millis();

        if (DEBUG) {
          Serial.print("I detected the keypress at index ");
          Serial.println(pressed);
          if (letterIndex == -1) {
            Serial.println("Got a no-connect key");
          } else {
            Serial.print("This corresponds to the keystroke: ");
            Serial.println(letters[letterIndex], HEX);
          }
        }

        if (is_primary || (!use_gatt && !is_connected)) {
          // Primary half always uses bleKB; wired secondary half falls back to bleKB when disconnected.
          Serial.println(letters[letterIndex]);
          if (not DUMMY) {
            bleKB.press(letters[letterIndex], letter_mods[letterIndex]);
          }
        } else if (use_gatt) {
          // Wireless secondary half: relay keypress to primary via GATT.
          gatt_send_key_press(letters[letterIndex], letter_mods[letterIndex]);
        } else if (split_keeb_communication) {
          Serial.print("Sending keystroke to partner: ");
          Serial.println(letters[letterIndex]);
          Serial2.write(press_flag);
          Serial2.write(letters[letterIndex]);
        }
      }

      // Subtract off again the keypress, so we can handle NKRO
      pressed -= i;
    }

    // Key is released
    if ((not keyStates[i]) and pKeyStates[i]) {
      pressed += i;

      int letterIndex = keys[pressed];
      if (DEBUG) {
        Serial.print("I detected a release at index ");
        Serial.println(pressed);
        Serial.print("This corresponds to the keystroke: ");
        Serial.println(letters[letterIndex], HEX);
      }

      if (is_primary || (!use_gatt && !is_connected)) {
        if (not DUMMY) {
          bleKB.release(letters[letterIndex]);
          if (letter_mods[letterIndex]) { bleKB.release(KEY_LSHIFT); }

          int letterIndex_alt = -1;
          // also release the layer below me or above me
          if (keyStates[MODKEY0] or locked_modkey) {
            letterIndex_alt = keys[pressed - NKEYS];
          } else {
            letterIndex_alt = keys[pressed + NKEYS];
          }

          if (letterIndex_alt != -1) {
            if (DEBUG) {
              Serial.print("Also releasing key at index: ");
              Serial.println(letterIndex_alt);
            }
            bleKB.release(letters[letterIndex_alt]);
          } else if (letterIndex_alt < -1) {
            // Handling Media keys
            letterIndex_alt *= -1;  // Make it positive

            bleKB.release(media_keys[letterIndex_alt]);

            if (DEBUG) {
              Serial.print("Also releasing Media key, index: ");
              Serial.println(letterIndex);
            }
          }
        }
      } else if (use_gatt) {
        // Wireless secondary half: relay key release to primary via GATT.
        gatt_send_key_release(letters[letterIndex], letter_mods[letterIndex]);
      } else if (split_keeb_communication) {
        Serial2.write(release_flag);
        Serial2.print(letters[letterIndex]);
      }

      // Subtract off again the keypress, so we can handle NKRO
      pressed -= i;
    }
  }
}

void send_keep_alive() {
  if (is_primary) {
    Serial.println("Sending keep alive...");
    Serial2.write(keep_alive);
    last_keep_alive_check = millis();
  }
}

void parse_other_half() {
  bool cont = true;
  while (cont) {
    if (Serial2.available()) {
      Serial.println("I have serial to parse...");
      int is_press = Serial2.read();

      // check if it's a keep alive message
      if (is_press == keep_alive) {
        Serial.println("Got a keep alive message! Still linked to the other half.");

        // Set flag and timer.
        is_connected = true;
        last_keep_alive_time = millis();

      } else if ((is_press == press_flag) or (is_press == release_flag)) {
        Serial.print("I got the message ");
        Serial.print(is_press);
        Serial.println(" so I expect a second message. Waiting for that now...");

        // Get the second half of the message
        while (not Serial2.available())
          ;
        uint8_t recv = uint8_t(Serial2.read());

        if (is_press == press_flag) {
          if (not DUMMY) { bleKB.press(recv); }
        } else if (is_press == release_flag) {
          bleKB.release(recv);
        }
      } else {
        Serial.println(is_press);
      }
    }

    cont = Serial2.available() > 0;
  }
}

void go_to_sleep() {
  if (DEBUG) { Serial.println("Entering deep sleep!"); }

  led_state = LOW;
  digitalWrite(LEDPin, led_state);

  // Clear the light-sleep timer wakeup so it doesn't fire during deep sleep
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);

  // Set which column pins can wake the device
  uint64_t buttonPinMask = 0;
  for (int i = 0; i < NWAKE; i++) {
    buttonPinMask |= (1ULL << wakePins[i]);
  }
  if (DEBUG) {
    Serial.print("Button pin mask is: ");
    Serial.println(buttonPinMask);
  }

  // Hold every row high so a pressed key can drive one of the selected wake columns high.
  for (int i = 0; i < NROWS; i++) {
    digitalWrite(rowPins[i], HIGH);
    gpio_hold_en((gpio_num_t)rowPins[i]);
  }
  gpio_deep_sleep_hold_en();

  for (int i = 0; i < NWAKE; i++) {
    gpio_num_t wake_pin = (gpio_num_t)wakePins[i];
    rtc_gpio_init(wake_pin);
    rtc_gpio_set_direction(wake_pin, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_dis(wake_pin);
    rtc_gpio_pulldown_en(wake_pin);
  }

  esp_sleep_enable_ext1_wakeup(buttonPinMask, ESP_EXT1_WAKEUP_ANY_HIGH);
  Serial.println("Going to sleep...");

  esp_deep_sleep_start();
}
