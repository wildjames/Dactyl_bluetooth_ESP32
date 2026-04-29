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
#include "BoardConfig_L.h"
// #include "BoardConfig_R.h"

#include "RuntimeState.h"
#include "MatrixScanner.h"

// Wireless GATT relay between halves (must come after BoardConfig so that
// bleKB and boardConfig are already declared).
#include "GattRelay.h"

//******************************************************************


RuntimeState runtimeState = {};
LinkState& linkState = runtimeState.link;

// For communicating with the other half - sends this message first
// to denote a press or release
const uint8_t press_flag = B00001111;
const uint8_t release_flag = B11110000;


// Function declarations
void initialize_debug_serial();
void detect_link_mode();
void configure_led_pwm();
void initialize_transport();
void initialize_runtime_timers();
bool refresh_primary_gatt_peer_state();
bool keyboard_is_active(bool primary_has_ble_peer);
void update_connected_led();
void update_disconnected_led();
void service_split_link();
void sleep_if_idle();
void update_modifier_tap_state();


void initialize_debug_serial() {
  if (!boardConfig.debug) {
    return;
  }

  Serial.begin(115200);
  delay(1000);
  Serial.print("BOOTED ");
  Serial.println(boardConfig.boardLabel);
}


void detect_link_mode() {
  if (detect_wired_connection()) {
    linkState.splitCommunication = true;
    linkState.useGatt = false;
    if (boardConfig.debug) { Serial.println("Connection: WIRED (Serial2)"); }
  } else {
    linkState.splitCommunication = false;
    linkState.useGatt = true;
    if (boardConfig.debug) { Serial.println("Connection: WIRELESS (GATT)"); }
  }
}


void configure_led_pwm() {
  ledcAttach(boardConfig.led.pin, boardConfig.led.frequency, boardConfig.led.resolution);
  runtimeState.led.dutyCycle = boardConfig.led.maxDutyCycle;
}


void initialize_transport() {
  // If we're wireless and the primary half, start the BLE keyboard immediately so we're discoverable during GATT connection.
  if (!linkState.useGatt && boardConfig.isPrimary) {
    bleKB.begin();
  }

  // If we're wireless, start the GATT bits
  if (!linkState.useGatt) {
    return;
  }

  if (boardConfig.isPrimary) {
    setup_gatt_server();
  } else {
    connect_to_primary_gatt();
  }
}


void initialize_runtime_timers() {
  unsigned long now = millis();
  runtimeState.modifiers.lastModTap = now;
  linkState.lastKeepAliveCheck = now;
  runtimeState.loop.lastKeypress = now;
  runtimeState.loop.lastLoop = now;
}


bool refresh_primary_gatt_peer_state() {
  // If we're not wireless, or not the primary, no-op
  if (!(linkState.useGatt && boardConfig.isPrimary)) {
    return false;
  }

  NimBLEServer* server = NimBLEDevice::getServer();
  if (server == nullptr) {
    return false;
  }

  // We expect either 0 or 1 connected peers (the secondary), but if we have more something's weird and we should just keep advertising.
  uint8_t connected_count = server->getConnectedCount();
  bool primary_has_ble_peer = connected_count > 0;
  NimBLEAdvertising* advertising = server->getAdvertising();
  if (advertising != nullptr
      && connected_count > 0
      && connected_count != linkState.lastGattConnectedCount
      && !advertising->isAdvertising()) {
    server->startAdvertising();
  }
  linkState.lastGattConnectedCount = connected_count;
  return primary_has_ble_peer;
}


bool keyboard_is_active(bool primary_has_ble_peer) {
  bool is_wireless_secondary = linkState.useGatt && !boardConfig.isPrimary;
  bool has_primary_ble_link = bleKB.isConnected() || bleKB.isPaired();
  bool can_primary_send_keys = has_primary_ble_link || primary_has_ble_peer;

  return is_wireless_secondary ? linkState.isConnected
                               : (can_primary_send_keys || linkState.isConnected);
}


void update_connected_led() {
  runtimeState.led.outputState = HIGH;
  if (runtimeState.modifiers.lockedModKey) {
    if (millis() - runtimeState.modifiers.lastModFlash >= 125) {
      runtimeState.modifiers.modFlashHigh = !runtimeState.modifiers.modFlashHigh;
      runtimeState.modifiers.lastModFlash = millis();
    }
    runtimeState.led.dutyCycle = runtimeState.modifiers.modFlashHigh
      ? boardConfig.led.maxDutyCycle
      : boardConfig.led.maxDutyCycle / 2;
  } else {
    runtimeState.led.dutyCycle = boardConfig.led.maxDutyCycle / 2;
  }

  ledcWrite(boardConfig.led.pin, runtimeState.led.outputState * runtimeState.led.dutyCycle);
}


void update_disconnected_led() {
  runtimeState.led.outputState = runtimeState.led.outputState == HIGH ? LOW : HIGH;
  ledcWrite(boardConfig.led.pin, runtimeState.led.outputState * boardConfig.led.maxDutyCycle);
}


void service_split_link() {
  if (!linkState.splitCommunication) {
    return;
  }

  if (millis() - linkState.lastKeepAliveCheck > boardConfig.timings.keepAliveDelayMs) {
    send_keep_alive();
  }

  if (millis() - boardConfig.timings.keepAliveLifespanMs > linkState.lastKeepAliveTime) {
    linkState.isConnected = false;
  }
}


void sleep_if_idle() {
  if (millis() - runtimeState.loop.lastKeypress > boardConfig.timings.deepSleepWaitMs) {
    go_to_sleep();
  }
}


void setup() {
  MatrixScanner::release_sleep_matrix_config(boardConfig);

  initialize_debug_serial();
  detect_link_mode();
  MatrixScanner::configure_pins(boardConfig);
  configure_led_pwm();
  initialize_transport();
  initialize_runtime_timers();

  update_battery_level();
}


void loop() {
  bool is_wireless_secondary = linkState.useGatt && !boardConfig.isPrimary;
  if (is_wireless_secondary && !linkState.isConnected) {
    connect_to_primary_gatt();
  }

  bool primary_has_ble_peer = refresh_primary_gatt_peer_state();
  bool keyboard_active = keyboard_is_active(primary_has_ble_peer);

  if (keyboard_active) {
    MatrixScanner::scan(boardConfig, runtimeState.matrix);
    update_modifier_tap_state();
    parse_keypress();
    if (linkState.splitCommunication) { parse_other_half(); }
    update_connected_led();
    service_split_link();

    int remaining_ms = boardConfig.timings.pollTimeMs - (int)(millis() - runtimeState.loop.lastLoop);
    if (remaining_ms > 1) {
      delay(remaining_ms);
    }

  } else {
    if (boardConfig.debug) { Serial.println("Not connected to bluetooth..."); }
    update_disconnected_led();
    if (linkState.splitCommunication) { parse_other_half(); }

    int remaining_ms = boardConfig.timings.disconnectedWaitMs - (int)(millis() - runtimeState.loop.lastLoop);
    if (remaining_ms > 1) {
      delay(remaining_ms);
    }

    if (millis() - runtimeState.loop.lastKeypress > boardConfig.timings.disconnectedDeepSleepMs) {
      go_to_sleep();
    }
  }

  if (millis() - runtimeState.battery.lastUpdate > boardConfig.timings.batteryPollIntervalMs) {
    update_battery_level();
  }

  sleep_if_idle();

  runtimeState.loop.lastLoop = millis();
}


void update_battery_level() {
  int battery_measurement = analogRead(boardConfig.battery.pin);
  float battery_voltage = battery_measurement * (boardConfig.battery.refVoltage * 2.0 * 1.1 / 4095);
  // min voltage is 3.2, max is 4.2
  float battery_percentage = 100.0 * (battery_voltage - boardConfig.battery.minVoltage)
                           / (boardConfig.battery.maxVoltage - boardConfig.battery.minVoltage);

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
  if (!(linkState.useGatt && !boardConfig.isPrimary)) {
    bleKB.setBatteryLevel(battery_percentage);
  }
  runtimeState.battery.lastUpdate = millis();
}


void update_modifier_tap_state() {
  MatrixState& matrix = runtimeState.matrix;

  // Remember what time we last pressed the modifier key
  if (matrix.previousKeyStates[MODKEY0] and (not matrix.keyStates[MODKEY0])) {
    if (boardConfig.debug) {
      Serial.println("Pressed the modifier key!");
      Serial.print("Time since last tap: ");
      Serial.println(millis() - runtimeState.modifiers.lastModTap);
    }
    // But first check if we got a double tap.
    if ((millis() - runtimeState.modifiers.lastModTap) < boardConfig.timings.doubleTapIntervalMs) {
      if (boardConfig.debug) {
        Serial.println("Toggling modifier lock");
      }
      runtimeState.modifiers.lockedModKey = !runtimeState.modifiers.lockedModKey;
      // Set the last tap to be somewhat in the past, so the next hit doesnt also trigger it
      runtimeState.modifiers.lastModTap -= boardConfig.timings.doubleTapIntervalMs;
    } else {
      // Then, save the current tap time.
      runtimeState.modifiers.lastModTap = millis();
    }
  }

  // Double-tap the shift key to toggle caps lock
  if (SHIFTKEY0 >= 0 && matrix.previousKeyStates[SHIFTKEY0] && (not matrix.keyStates[SHIFTKEY0])) {
    if ((millis() - runtimeState.modifiers.lastShiftTap) < boardConfig.timings.doubleTapIntervalMs) {
      if (boardConfig.debug) { Serial.println("Toggling caps lock"); }
      bleKB.tap(KEY_CAPS_LOCK);
      runtimeState.modifiers.lastShiftTap -= boardConfig.timings.doubleTapIntervalMs;
    } else {
      runtimeState.modifiers.lastShiftTap = millis();
    }
  }
}


void parse_keypress() {
  if (runtimeState.modifiers.isAltLayout) {
    parse_alt();
  } else {
    parse_typing();
  }
}

void parse_alt() {
  // Handle toggling back to typing
  if (typing_toggle >= 0) {
    if (runtimeState.matrix.keyStates[typing_toggle]) {
      Serial.println("\nSWAPPING TO TYPING MODE\n");
      runtimeState.modifiers.isAltLayout = false;

      // release all keys
      bleKB.releaseAll();
      // To prevent the toggle key firing its key on the new layer, set its pState to True
      runtimeState.matrix.previousKeyStates[typing_toggle] = 1;

      return;
    }
  }

  send_keypress(alt_keymap);
}

void parse_typing() {
  if (alt_toggle >= 0) {
    if (runtimeState.matrix.keyStates[alt_toggle]) {
      Serial.println("\nSWAPPING TO alt MODE\n");
      runtimeState.modifiers.isAltLayout = true;

      // release all keys
      bleKB.releaseAll();
      return;
    }
  }

  send_keypress(keymap);
}


void send_keypress(int keys[]) {
  MatrixState& matrix = runtimeState.matrix;

  int pressed = 0;
  // Handle layering
  if (matrix.keyStates[MODKEY0] or runtimeState.modifiers.lockedModKey) {
    pressed += NKEYS;
  }

  for (int i = 0; i < NKEYS; i++) {
    if (matrix.keyStates[i] and (not matrix.previousKeyStates[i])) {
      pressed += i;
      int letterIndex = keys[pressed];

      // Skip NC keys
      if (letterIndex == -1) {
        if (boardConfig.debug) {
          Serial.println("Got a no-connect key");
        }
      } else if (letterIndex < -1) {
        // Handling Media keys
        letterIndex *= -1;  // Make it positive

        if (boardConfig.isPrimary || (!linkState.useGatt && !linkState.isConnected)) {
          if (!boardConfig.dummy) {
            bleKB.tap(media_keys[letterIndex]);
          }
        } else if (linkState.useGatt) {
          gatt_send_media_key(media_keys[letterIndex]);
        }

        if (boardConfig.debug) {
          Serial.print("Detected a keypress at Media key, index: ");
          Serial.println(letterIndex);
        }
      } else {
        runtimeState.loop.lastKeypress = millis();

        if (boardConfig.debug) {
          Serial.print("I detected the keypress at index ");
          Serial.println(pressed);
          if (letterIndex == -1) {
            Serial.println("Got a no-connect key");
          } else {
            Serial.print("This corresponds to the keystroke: ");
            Serial.println(letters[letterIndex], HEX);
          }
        }

        if (boardConfig.isPrimary || (!linkState.useGatt && !linkState.isConnected)) {
          // Primary half always uses bleKB; wired secondary half falls back to bleKB when disconnected.
          Serial.println(letters[letterIndex]);
          if (!boardConfig.dummy) {
            bleKB.press(letters[letterIndex], letter_mods[letterIndex]);
          }
        } else if (linkState.useGatt) {
          // Wireless secondary half: relay keypress to primary via GATT.
          gatt_send_key_press(letters[letterIndex], letter_mods[letterIndex]);
        } else if (linkState.splitCommunication) {
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
    if ((not matrix.keyStates[i]) and matrix.previousKeyStates[i]) {
      pressed += i;

      int letterIndex = keys[pressed];
      if (boardConfig.debug) {
        Serial.print("I detected a release at index ");
        Serial.println(pressed);
        Serial.print("This corresponds to the keystroke: ");
        Serial.println(letters[letterIndex], HEX);
      }

      if (boardConfig.isPrimary || (!linkState.useGatt && !linkState.isConnected)) {
        if (!boardConfig.dummy) {
          bleKB.release(letters[letterIndex]);
          if (letter_mods[letterIndex]) { bleKB.release(KEY_LSHIFT); }

          int letterIndex_alt = -1;
          // also release the layer below me or above me
          if (matrix.keyStates[MODKEY0] or runtimeState.modifiers.lockedModKey) {
            letterIndex_alt = keys[pressed - NKEYS];
          } else {
            letterIndex_alt = keys[pressed + NKEYS];
          }

          if (letterIndex_alt != -1) {
            if (boardConfig.debug) {
              Serial.print("Also releasing key at index: ");
              Serial.println(letterIndex_alt);
            }
            bleKB.release(letters[letterIndex_alt]);
          } else if (letterIndex_alt < -1) {
            // Handling Media keys
            letterIndex_alt *= -1;  // Make it positive

            bleKB.release(media_keys[letterIndex_alt]);

            if (boardConfig.debug) {
              Serial.print("Also releasing Media key, index: ");
              Serial.println(letterIndex);
            }
          }
        }
      } else if (linkState.useGatt) {
        // Wireless secondary half: relay key release to primary via GATT.
        gatt_send_key_release(letters[letterIndex], letter_mods[letterIndex]);
      } else if (linkState.splitCommunication) {
        Serial2.write(release_flag);
        Serial2.print(letters[letterIndex]);
      }

      // Subtract off again the keypress, so we can handle NKRO
      pressed -= i;
    }
  }
}

void send_keep_alive() {
  if (boardConfig.isPrimary) {
    Serial.println("Sending keep alive...");
    Serial2.write(linkState.keepAliveMessage);
    linkState.lastKeepAliveCheck = millis();
  }
}

void parse_other_half() {
  bool cont = true;
  while (cont) {
    if (Serial2.available()) {
      Serial.println("I have serial to parse...");
      int is_press = Serial2.read();

      // check if it's a keep alive message
      if (is_press == linkState.keepAliveMessage) {
        Serial.println("Got a keep alive message! Still linked to the other half.");

        // Set flag and timer.
        linkState.isConnected = true;
        linkState.lastKeepAliveTime = millis();

      } else if ((is_press == press_flag) or (is_press == release_flag)) {
        Serial.print("I got the message ");
        Serial.print(is_press);
        Serial.println(" so I expect a second message. Waiting for that now...");

        // Get the second half of the message
        while (not Serial2.available())
          ;
        uint8_t recv = uint8_t(Serial2.read());

        if (is_press == press_flag) {
          if (!boardConfig.dummy) { bleKB.press(recv); }
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
  if (boardConfig.debug) { Serial.println("Entering deep sleep!"); }

  runtimeState.led.outputState = LOW;
  digitalWrite(boardConfig.led.pin, runtimeState.led.outputState);

  // Clear the light-sleep timer wakeup so it doesn't fire during deep sleep
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);

  uint64_t buttonPinMask = MatrixScanner::build_wake_pin_mask(boardConfig);
  if (boardConfig.debug) {
    Serial.print("Button pin mask is: ");
    Serial.println(buttonPinMask);
  }

  MatrixScanner::prepare_wake_pins(boardConfig);

  esp_sleep_enable_ext1_wakeup(buttonPinMask, ESP_EXT1_WAKEUP_ANY_HIGH);
  Serial.println("Going to sleep...");

  esp_deep_sleep_start();
}
