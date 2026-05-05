#include "LinkManager.h"

#include "BoardConfig.h"
#include "GattRelay.h"
#include "HidDispatcher.h"

#include "tusb.h"

namespace {

bool primaryBlePeerConnected = false;

}

namespace LinkManager {


// Unfortunately, this function is necessarily complex.
//
// In a nutshell, these are the main scenarios it needs to handle:
// 1. Primary and secondary connected via GATT, then primary USB is plugged in (primary should switch to USB HID, but keep GATT alive so the secondary can still send events).
// 2. Primary and secondary connected via GATT, then primary USB is unplugged (primary should switch back to BLE HID, secondary should reconnect via GATT if needed).
//
// 3. Secondary is connected to primary via GATT, then secondary is plugged into USB (primary should remain in BLE HID, and secondary should switch to USB HID and disconnect from primary).
// 4. Secondary is connected to USB HID, then unplugged (secondary should switch back to BLE HID and reconnect to primary).
//
// 5. Primary and secondary connected via GATT, then primary is plugged into USB, then secondary is plugged into USB (primary should switch to USB HID and stop advertising BLE HID, secondary should switch to USB HID and disconnect from primary).
//
// Throughout all this, the primary should _always_ be advertising the GATT server, waiting for a possible secondary or maintaining an existing secondary.
void check_if_usb_connected(LinkState& state) {

  if (tud_connected() && state.connectionType != ConnectionType::USB) {
    // USB just connected
    if (boardConfig.debug) {
      Serial.println("Connected to USB host — switching to USB HID");
    }
    HidDispatcher::begin_usb();
    state.connectionType = ConnectionType::USB;

    if (state.allowGatt) {
      if (!boardConfig.isPrimary) {
        // Primary: keep the NimBLE server and GATT advertising alive so the
        // secondary can still connect/relay keystrokes. Only the BLE HID host
        // connection is superseded by USB — the relay service stays up.
        if (boardConfig.debug) {
          Serial.println("GATT relay kept alive (USB active for HID output)");
        }

      } else {
        // Secondary: fully disconnect from primary — no relay needed when wired.
        std::vector<NimBLEClient*> clients = NimBLEDevice::getConnectedClients();
        for (NimBLEClient* pClient : clients) {
          pClient->disconnect();
        }
        if (boardConfig.debug) {
          Serial.println("BLE client disconnected (USB active)");
        }
      }
    }

  } else if (!tud_connected() && state.connectionType == ConnectionType::USB) {
    // Not connected to USB, but was previously - it was unplugged.
    state.connectionType = ConnectionType::None;

    // Restart BLE so the keyboard is discoverable/connected again
    if (state.allowGatt) {
      if (boardConfig.isPrimary) {
        // GATT server was never stopped — nothing to restart.
        // Just resume BLE HID output on next tick.
        if (boardConfig.debug) {
          Serial.println("Resuming BLE HID output");
        }
      } else {
        connect_to_primary_gatt();
      }
      state.connectionType = ConnectionType::Bluetooth;
    }
  }
}

void begin(LinkState& state) {
  state.allowGatt = true;

  if (boardConfig.debug) {
    Serial.println("Connection: WIRELESS (GATT)");
  }

  if (boardConfig.isPrimary) {
    // Primary always needs HID — bleKB.begin() initialises NimBLE.
    HidDispatcher::begin();
  }

  state.connectionType = ConnectionType::None;
  if (state.allowGatt) {
    if (boardConfig.isPrimary) {
      // Attach relay service to the server that bleKB already owns.
      setup_gatt_server();
    } else {
      connect_to_primary_gatt();
    }
    state.connectionType = ConnectionType::Bluetooth;

  } else {
    check_if_usb_connected(state);
  }
}

void primary_tick(LinkState& state) {
  check_if_usb_connected(state);

  // Always maintain GATT relay regardless of USB state — the secondary may
  // still be sending keystrokes via GATT even while the primary uses USB HID.
  primaryBlePeerConnected = false;
  if (state.allowGatt) {
    NimBLEServer* server = NimBLEDevice::getServer();
    if (server != nullptr) {
      uint8_t connectedCount = server->getConnectedCount();
      primaryBlePeerConnected = connectedCount > 0;
      NimBLEAdvertising* advertising = server->getAdvertising();
      if (advertising != nullptr
          && connectedCount != state.lastGattConnectedCount
          && !advertising->isAdvertising()) {
        server->startAdvertising();
      }
      state.lastGattConnectedCount = connectedCount;
    }
  }
}

void secondary_tick(LinkState& state) {
  check_if_usb_connected(state);
  if (state.connectionType == ConnectionType::USB) {
    return;
  }

  // If we're the secondary, and disconnected, try to connect to the primary.
  if (state.allowGatt && state.connectionType == ConnectionType::None) {
    connect_to_primary_gatt();
  }
}

void tick(LinkState& state, LedState& ledState) {
  if (boardConfig.isPrimary) {
    primary_tick(state);
  } else {
    secondary_tick(state);
  }

  if (state.connectionType != ConnectionType::None) {
    ledState.mode = LedMode::Connected;
  } else {
    ledState.mode = LedMode::Disconnected;
  }
}

void poll_incoming(LinkState& state, bool dummy) {
  // Placeholder for the primary to poll the secondary for key events
  // GATT messages are handled in callbacks, but other comms methods may not be.
  (void)state;
  (void)dummy;
}

bool has_primary_ble_peer() {
  return primaryBlePeerConnected;
}

bool dispatch_remote_action(const KeymapResolver::Action& action, const LinkState& state) {
  if (state.allowGatt) {
    switch (action.type) {
      case KeymapResolver::ActionType::KeyTap:
        gatt_send_tap_key(action.keycode);
        return true;

      case KeymapResolver::ActionType::KeyPress:
        gatt_send_key_press(action.keycode);
        return true;

      case KeymapResolver::ActionType::KeyRelease:
        gatt_send_key_release(action.keycode);
        return true;

      case KeymapResolver::ActionType::None:
      case KeymapResolver::ActionType::ReleaseAll:
      case KeymapResolver::ActionType::TapCapsLock:
        return false;
    }
  }

  return false;
}

}
