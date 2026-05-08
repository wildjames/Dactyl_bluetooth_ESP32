// HijelHID_BLEKeyboard.h #defines HID_REPORT_ID_KEYBOARD as a numeric
// constant. The ESP32 USB library's USBHID.h uses the same name as an enum
// value. We must #undef the macro between the two includes to avoid a conflict.
#include "HidDispatcher.h"

#include <HijelHID_BLEKeyboard.h>

#undef HID_REPORT_ID_KEYBOARD
#undef HID_REPORT_ID_CONSUMER
#undef HID_REPORT_ID_GAMEPAD

#include "USB.h"
#include "USBHIDKeyboard.h"
#include "tusb.h"

namespace {

HijelHID_BLEKeyboard* bleKB = nullptr;
USBHIDKeyboard usbKB;
bool usbStarted = false;

// Returns true only when USB is started AND the host is actively listening
// (mounted + not suspended). tud_connected() alone can't detect cable unplug
// on boards without VBUS sensing — SOF timeout via tud_ready() catches it.
bool usb_can_send() {
  return usbStarted && tud_ready();
}

// Decides whether to use USB for this call. On the primary, USB might have
// been unplugged (tud_ready() == false) and we fall back to BLE. On the
// secondary, BLE HID is never initialised so we must always use USB when
// the connection type says USB.
bool should_use_usb(ConnectionType conn) {
  if (conn != ConnectionType::USB) return false;
  if (boardConfig.isPrimary) return usb_can_send();
  return usbStarted;  // secondary: always send via USB if started
}

}

namespace HidDispatcher {

void begin() {
  if (!bleKB) {
    bleKB = new HijelHID_BLEKeyboard(boardConfig.bleDeviceName, boardConfig.manufacturerName, 50);
  }
  bleKB->begin();
}

void begin_usb() {
  if (!usbStarted) {
    usbKB.begin();
    USB.begin();
    usbStarted = true;
    if (boardConfig.debug) {
      Serial.println("[HID] USB HID started");
    }
  }
}

bool has_host_connection() {
  return bleKB && (bleKB->isConnected() || bleKB->isPaired());
}

bool has_usb_connection() {
  return usbStarted;
}

void set_battery_level(float batteryPercentage) {
  bleKB->setBatteryLevel(batteryPercentage);
}

void release_all(ConnectionType conn) {
  if (should_use_usb(conn)) {
    usbKB.releaseAll();
  } else {
    bleKB->releaseAll();
  }
}

void tap_key(uint8_t keyCode, bool dummy, ConnectionType conn) {
  if (dummy) {
    return;
  }

  if (should_use_usb(conn)) {
    usbKB.pressRaw(keyCode);
    delay(25);
    usbKB.releaseAll();
  } else {
    bleKB->tap(keyCode);
  }
}

void press_key(uint8_t keycode, bool dummy, ConnectionType conn) {
  if (dummy) {
    return;
  }

  if (should_use_usb(conn)) {
    if (boardConfig.debug) {
      Serial.print("[HID] USB pressRaw keycode=");
      Serial.print(keycode);
      Serial.print(" tud_mounted=");
      Serial.print(tud_mounted());
      Serial.print(" tud_ready=");
      Serial.println(tud_ready());
    }
    usbKB.pressRaw(keycode);
  } else {
    bleKB->press(keycode);
  }
}

void release_key(uint8_t keycode, bool dummy, ConnectionType conn) {
  if (dummy) {
    return;
  }

  if (should_use_usb(conn)) {
    usbKB.releaseAll();
  } else {
    bleKB->release(keycode);
  }
}

void dispatch_action(const KeymapResolver::Action& action, bool dummy, ConnectionType conn) {
  switch (action.type) {
    case KeymapResolver::ActionType::None:
      return;

    case KeymapResolver::ActionType::ReleaseAll:
      release_all(conn);
      return;

    case KeymapResolver::ActionType::KeyTap:
      tap_key(action.keycode, dummy, conn);
      return;

    case KeymapResolver::ActionType::KeyPress:
      press_key(action.keycode, dummy, conn);
      return;

    case KeymapResolver::ActionType::KeyRelease:
      release_key(action.keycode, dummy, conn);
      return;
  }
}

void before_sleep() {
  if (bleKB) {
    bleKB->beforeSleep();
  }
}

}
