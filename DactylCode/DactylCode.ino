#include <HijelHID_BLEKeyboard.h>


//******************************************************************

// board-specific info in a header file. Make sure to change this!
#include "config/BoardConfig_L.h"
// #include "config/BoardConfig_R.h"

#include "RuntimeState.h"
#include "MatrixScanner.h"
#include "KeymapResolver.h"
#include "HidDispatcher.h"
#include "LinkManager.h"
#include "PowerManager.h"
#include "StatusLed.h"

//******************************************************************


RuntimeState runtimeState = {};
LinkState& linkState = runtimeState.link;
KeymapResolver::KeyboardState keyboardState = {};


// Function declarations
void initialize_debug_serial();
void initialize_runtime_timers();
bool keyboard_is_active(bool primary_has_ble_peer);
void sleep_if_idle();
void dispatch_keymap_result(const KeymapResolver::Result& result);
void dispatch_keymap_action(const KeymapResolver::Action& action);

KeymapResolver::Config make_keymap_resolver_config() {
  KeymapResolver::Config config = {};
  config.modifierKeyIndex = MODKEY0;
  config.shiftKeyIndex = SHIFTKEY0;
  config.altToggleKeyIndex = alt_toggle;
  config.typingToggleKeyIndex = typing_toggle;
  config.doubleTapIntervalMs = boardConfig.timings.doubleTapIntervalMs;
  config.doubleTapMinIntervalMs = boardConfig.timings.doubleTapMinIntervalMs;
  config.primaryKeymap = keymap;
  config.primaryKeymapLength = sizeof(keymap) / sizeof(keymap[0]);
  config.alternateKeymap = alt_keymap;
  config.alternateKeymapLength = sizeof(alt_keymap) / sizeof(alt_keymap[0]);
  return config;
}

const KeymapResolver::Config keymapResolverConfig = make_keymap_resolver_config();


void initialize_debug_serial() {
  if (!boardConfig.debug) {
    return;
  }

  Serial.begin(115200);
  delay(1000);
  Serial.print("BOOTED ");
  Serial.println(boardConfig.boardLabel);
}

void initialize_runtime_timers() {
  unsigned long now = millis();
  keyboardState.lastModTap = now;
  keyboardState.lastShiftTap = now;
  keyboardState.lastKeypress = now;
  runtimeState.loop.lastLoop = now;
  runtimeState.loop.lastActivity = now;
}

bool keyboard_is_active(bool primary_has_ble_peer) {
  // Wired, then always active
  if (linkState.connectionType == ConnectionType::USB) {
    return true;
  }

  // Otherwise, check that the pimary/secondary link is up and report accordingly
  bool is_wireless_secondary = linkState.allowGatt && !boardConfig.isPrimary;
  bool has_primary_ble_link = HidDispatcher::has_host_connection();
  bool can_primary_send_keys = has_primary_ble_link || primary_has_ble_peer;

  return is_wireless_secondary ? (linkState.connectionType != ConnectionType::None)
                               : can_primary_send_keys;
}

bool is_battery_charging() {
  // If battery monitor reports a positive charge rate, we're charging.
  if (runtimeState.battery.monitorAvailable && runtimeState.battery.chargeRate > 0.0f) {
    return true;
  }
  // USB connection implies external power.
  if (linkState.connectionType == ConnectionType::USB) {
    return true;
  }
  return false;
}

void sleep_if_idle() {
  // Never sleep while charging.
  if (is_battery_charging()) { return; }

  if (millis() - runtimeState.loop.lastActivity > boardConfig.timings.deepSleepWaitMs) {
    PowerManager::enter_deep_sleep(boardConfig, runtimeState.led);
  }
}


void setup() {
  MatrixScanner::release_sleep_matrix_config(boardConfig);

  initialize_debug_serial();

  // Initialise USB HID early — on ESP32-S3 the device is physically connected
  // as soon as the board powers on. If we delay USB.begin() the host may time
  // out on enumeration.
  HidDispatcher::begin_usb();

  MatrixScanner::configure_pins(boardConfig);
  StatusLed::begin(boardConfig, runtimeState.led);
  PowerManager::begin(boardConfig, runtimeState.battery);
  LinkManager::begin(linkState);
  initialize_runtime_timers();
}


void loop() {
  LinkManager::tick(linkState, runtimeState.led);
  bool primary_has_ble_peer = LinkManager::has_primary_ble_peer();
  bool keyboard_active = keyboard_is_active(primary_has_ble_peer);

  if (keyboard_active) {
    MatrixScanner::scan(boardConfig, runtimeState.matrix);

    // if debugging, print any newly pressed keys
    if (boardConfig.debug) {
      for (int i = 0; i < MATRIX_KEY_COUNT; i++) {
        if (runtimeState.matrix.keyStates[i] && !runtimeState.matrix.previousKeyStates[i]) {
          Serial.print("Key pressed: ");
          Serial.println(i);
        }
      }
    }

    KeymapResolver::Result keymapResult = {};
    KeymapResolver::resolve(runtimeState.matrix, keyboardState, keymapResolverConfig, keymapResult, runtimeState.led);
    dispatch_keymap_result(keymapResult);

    // Update the canonical activity timer and notify the other half.
    if (keymapResult.actionCount > 0) {
      runtimeState.loop.lastActivity = millis();
      if (boardConfig.isPrimary) {
        LinkManager::notify_activity();
      }
    }

    LinkManager::poll_incoming(linkState, boardConfig.dummy);

    int remaining_ms = boardConfig.timings.pollTimeMs - (int)(millis() - runtimeState.loop.lastLoop);
    if (remaining_ms > 1) {
      delay(remaining_ms);
    }

  } else {
    if (boardConfig.debug) { Serial.println("Not connected to bluetooth..."); }
    LinkManager::poll_incoming(linkState, boardConfig.dummy);

    int remaining_ms = boardConfig.timings.disconnectedWaitMs - (int)(millis() - runtimeState.loop.lastLoop);
    if (remaining_ms > 1) {
      delay(remaining_ms);
    }
  }

  if (millis() - runtimeState.battery.lastUpdate > boardConfig.timings.batteryPollIntervalMs) {
    PowerManager::update_battery_level(boardConfig, linkState, runtimeState.battery);
  }

  StatusLed::update(boardConfig, runtimeState.led);
  sleep_if_idle();

  runtimeState.loop.lastLoop = millis();
}


void dispatch_keymap_result(const KeymapResolver::Result& result) {
  for (int i = 0; i < result.actionCount; i++) {
    if (boardConfig.debug) {
      Serial.print("Dispatching action ");
      Serial.print(i+1);
      Serial.print("/");
      Serial.print(result.actionCount);
      Serial.print(" using connection type ");
      Serial.print((int)linkState.connectionType);
      Serial.print(": type=");
      Serial.print((int)result.actions[i].type);
      Serial.print(", keyIndex=");
      Serial.print(result.actions[i].keyIndex);
      Serial.print(", keycode=");
      Serial.println(result.actions[i].keycode);
    }
    dispatch_keymap_action(result.actions[i]);
  }
}

void dispatch_keymap_action(const KeymapResolver::Action& action) {
  bool use_local_hid = boardConfig.isPrimary
                       || linkState.connectionType == ConnectionType::USB
                       || (!linkState.allowGatt && linkState.connectionType == ConnectionType::None);

  switch (action.type) {
    case KeymapResolver::ActionType::None:
      return;

    case KeymapResolver::ActionType::ReleaseAll:
    case KeymapResolver::ActionType::KeyTap:
    case KeymapResolver::ActionType::KeyPress:
    case KeymapResolver::ActionType::KeyRelease:
      if (use_local_hid) {
        HidDispatcher::dispatch_action(action, boardConfig.dummy, linkState.connectionType);
      } else {
        LinkManager::dispatch_remote_action(action, linkState);
      }
      return;
  }
}

