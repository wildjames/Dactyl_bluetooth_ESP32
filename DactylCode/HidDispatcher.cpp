// HidDispatcher.h pulls in HijelHID_BLEKeyboard.h which #defines
// HID_REPORT_ID_KEYBOARD as a numeric constant. The ESP32 USB library's
// USBHID.h uses the same name as an enum value. We must #undef the macro
// between the two includes to avoid a conflict.
#include "HidDispatcher.h"

#undef HID_REPORT_ID_KEYBOARD
#undef HID_REPORT_ID_CONSUMER
#undef HID_REPORT_ID_GAMEPAD

#include "USB.h"
#include "USBHIDKeyboard.h"
#include "tusb.h"

namespace {

USBHIDKeyboard usbKB;
bool usbStarted = false;

// Returns true only when USB is started AND the host is actively listening
// (mounted + not suspended). tud_connected() alone can't detect cable unplug
// on boards without VBUS sensing — SOF timeout via tud_ready() catches it.
bool usb_can_send() {
  return usbStarted && tud_ready();
}

}

namespace HidDispatcher {

void begin() {
  bleKB.begin();
}

void begin_usb() {
  if (!usbStarted) {
    USB.begin();
    usbKB.begin();
    usbStarted = true;
  }
}

bool has_host_connection() {
  return bleKB.isConnected() || bleKB.isPaired();
}

bool has_usb_connection() {
  return usbStarted;
}

void set_battery_level(float batteryPercentage) {
  bleKB.setBatteryLevel(batteryPercentage);
}

void release_all(ConnectionType conn) {
  if (conn == ConnectionType::USB && usb_can_send()) {
    usbKB.releaseAll();
  } else {
    bleKB.releaseAll();
  }
}

void tap_caps_lock(ConnectionType conn) {
  if (conn == ConnectionType::USB && usb_can_send()) {
    usbKB.pressRaw((uint8_t)KEY_CAPS_LOCK);
    delay(25);
    usbKB.releaseAll();
  } else {
    bleKB.tap((uint8_t)KEY_CAPS_LOCK);
  }
}

void tap_key(uint8_t keyCode, bool dummy, ConnectionType conn) {
  if (dummy) {
    return;
  }

  if (conn == ConnectionType::USB && usb_can_send()) {
    usbKB.pressRaw(keyCode);
    delay(25);
    usbKB.releaseAll();
  } else {
    bleKB.tap(keyCode);
  }
}

void press_key(uint8_t keycode, bool dummy, ConnectionType conn) {
  if (dummy) {
    return;
  }

  if (conn == ConnectionType::USB && usb_can_send()) {
    usbKB.pressRaw(keycode);
  } else {
    bleKB.press(keycode);
  }
}

void release_key(uint8_t keycode, bool dummy, ConnectionType conn) {
  if (dummy) {
    return;
  }

  if (conn == ConnectionType::USB && usb_can_send()) {
    usbKB.releaseAll();
  } else {
    bleKB.release(keycode);
  }
}

void press_passthrough(uint8_t keycode, bool dummy, ConnectionType conn) {
  if (dummy) {
    return;
  }

  if (conn == ConnectionType::USB && usb_can_send()) {
    usbKB.pressRaw(keycode);
  } else {
    bleKB.press(keycode);
  }
}

void release_passthrough(uint8_t keycode, ConnectionType conn) {
  if (conn == ConnectionType::USB && usb_can_send()) {
    usbKB.releaseAll();
  } else {
    bleKB.release(keycode);
  }
}

void dispatch_action(const KeymapResolver::Action& action, bool dummy, ConnectionType conn) {
  switch (action.type) {
    case KeymapResolver::ActionType::None:
      return;

    case KeymapResolver::ActionType::ReleaseAll:
      release_all(conn);
      return;

    case KeymapResolver::ActionType::TapCapsLock:
      tap_caps_lock(conn);
      return;

    case KeymapResolver::ActionType::KeyPress:
      press_key(action.keycode, dummy, conn);
      return;

    case KeymapResolver::ActionType::KeyRelease:
      release_key(action.keycode, dummy, conn);
      return;
  }
}

}
