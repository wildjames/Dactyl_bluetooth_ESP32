#include <HijelHID_BLEKeyboard.h>


//******************************************************************

// board-specific info in a header file. Make sure to change this!
// #include "config/BoardConfig_L.h"
#include "config/BoardConfig_R.h"

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
}

bool keyboard_is_active(bool primary_has_ble_peer) {
  bool is_wireless_secondary = linkState.allowGatt && !boardConfig.isPrimary;
  bool has_primary_ble_link = HidDispatcher::has_host_connection();
  bool can_primary_send_keys = has_primary_ble_link || primary_has_ble_peer;

  return is_wireless_secondary ? linkState.isConnected
                               : (can_primary_send_keys || linkState.isConnected);
}

void sleep_if_idle() {
  if (millis() - keyboardState.lastKeypress > boardConfig.timings.deepSleepWaitMs) {
    PowerManager::enter_deep_sleep(boardConfig, runtimeState.led);
  }
}


void setup() {
  MatrixScanner::release_sleep_matrix_config(boardConfig);

  initialize_debug_serial();
  MatrixScanner::configure_pins(boardConfig);
  StatusLed::begin(boardConfig, runtimeState.led);
  LinkManager::begin(linkState);
  PowerManager::begin(boardConfig, runtimeState.battery);
  initialize_runtime_timers();

  PowerManager::update_battery_level(boardConfig, linkState, runtimeState.battery);
}


void loop() {
  LinkManager::tick(linkState);
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
    KeymapResolver::resolve(runtimeState.matrix, keyboardState, keymapResolverConfig, keymapResult);
    dispatch_keymap_result(keymapResult);
    LinkManager::poll_incoming(linkState, boardConfig.dummy);
    StatusLed::show_connected(boardConfig, runtimeState.led, keyboardState);

    int remaining_ms = boardConfig.timings.pollTimeMs - (int)(millis() - runtimeState.loop.lastLoop);
    if (remaining_ms > 1) {
      delay(remaining_ms);
    }

  } else {
    if (boardConfig.debug) { Serial.println("Not connected to bluetooth..."); }
    StatusLed::show_disconnected(boardConfig, runtimeState.led);
    LinkManager::poll_incoming(linkState, boardConfig.dummy);

    int remaining_ms = boardConfig.timings.disconnectedWaitMs - (int)(millis() - runtimeState.loop.lastLoop);
    if (remaining_ms > 1) {
      delay(remaining_ms);
    }

    if (millis() - keyboardState.lastKeypress > boardConfig.timings.disconnectedDeepSleepMs) {
      PowerManager::enter_deep_sleep(boardConfig, runtimeState.led);
    }
  }

  if (millis() - runtimeState.battery.lastUpdate > boardConfig.timings.batteryPollIntervalMs) {
    PowerManager::update_battery_level(boardConfig, linkState, runtimeState.battery);
  }

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
  bool use_local_hid = boardConfig.isPrimary || (!linkState.allowGatt && !linkState.isConnected);

  switch (action.type) {
    case KeymapResolver::ActionType::None:
      return;

    // These actions are local to the board and don't need to be forwarded
    case KeymapResolver::ActionType::ReleaseAll:
    case KeymapResolver::ActionType::TapCapsLock:
      if (use_local_hid) {
        HidDispatcher::dispatch_action(action, boardConfig.dummy);
      } else {
        LinkManager::dispatch_remote_action(action, linkState);
      }
      return;

    case KeymapResolver::ActionType::KeyPress:
      if (use_local_hid) {
        HidDispatcher::dispatch_action(action, boardConfig.dummy);
      } else {
        LinkManager::dispatch_remote_action(action, linkState);
      }
      return;

    case KeymapResolver::ActionType::KeyRelease:
      if (use_local_hid) {
        HidDispatcher::dispatch_action(action, boardConfig.dummy);
      } else {
        LinkManager::dispatch_remote_action(action, linkState);
      }
      return;
  }
}

