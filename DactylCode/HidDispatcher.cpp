#include "HidDispatcher.h"

namespace HidDispatcher {

void begin() {
  bleKB.begin();
}

bool has_host_connection() {
  return bleKB.isConnected() || bleKB.isPaired();
}

void set_battery_level(float batteryPercentage) {
  bleKB.setBatteryLevel(batteryPercentage);
}

void release_all() {
  bleKB.releaseAll();
}

void tap_caps_lock() {
  bleKB.tap(KEY_CAPS_LOCK);
}

void tap_media(uint16_t mediaCode, bool dummy) {
  if (dummy) {
    return;
  }

  bleKB.tap(mediaCode);
}

void release_media(uint16_t mediaCode, bool dummy) {
  if (dummy) {
    return;
  }

  bleKB.release(mediaCode);
}

void press_key(uint8_t keycode, uint8_t modifier, bool dummy) {
  if (dummy) {
    return;
  }

  bleKB.press(keycode, modifier);
}

void release_key(uint8_t keycode, uint8_t modifier, bool dummy) {
  if (dummy) {
    return;
  }

  bleKB.release(keycode);
  if (modifier) {
    bleKB.release(KEY_LSHIFT);
  }
}

void press_passthrough(uint8_t keycode, bool dummy) {
  if (dummy) {
    return;
  }

  bleKB.press(keycode);
}

void release_passthrough(uint8_t keycode) {
  bleKB.release(keycode);
}

void dispatch_action(const KeymapResolver::Action& action, bool dummy) {
  switch (action.type) {
    case KeymapResolver::ActionType::None:
      return;

    case KeymapResolver::ActionType::ReleaseAll:
      release_all();
      return;

    case KeymapResolver::ActionType::TapCapsLock:
      tap_caps_lock();
      return;

    case KeymapResolver::ActionType::MediaTap:
      tap_media(action.mediaCode, dummy);
      return;

    case KeymapResolver::ActionType::MediaRelease:
      release_media(action.mediaCode, dummy);
      return;

    case KeymapResolver::ActionType::KeyPress:
      press_key(action.keycode, action.modifier, dummy);
      return;

    case KeymapResolver::ActionType::KeyRelease:
      release_key(action.keycode, action.modifier, dummy);
      return;
  }
}

}
