#include "LinkManager.h"

#include "BoardConfig.h"
#include "GattRelay.h"
#include "HidDispatcher.h"

namespace {

bool primaryBlePeerConnected = false;

}

namespace LinkManager {

void begin(LinkState& state) {
  state.allowGatt = true;

  if (boardConfig.debug) {
    Serial.println("Connection: WIRELESS (GATT)");
  }

  if (boardConfig.isPrimary) {
    // Primary always needs HID — bleKB.begin() initialises NimBLE.
    HidDispatcher::begin();
  }

  if (state.allowGatt) {
    if (boardConfig.isPrimary) {
      // Attach relay service to the server that bleKB already owns.
      setup_gatt_server();
    } else {
      connect_to_primary_gatt();
    }
  } else if (!boardConfig.isPrimary) {
    // Secondary without GATT still needs its own HID device.
    HidDispatcher::begin();
  }
}

void primary_tick(LinkState& state) {
  primaryBlePeerConnected = false;
  if (state.allowGatt) {
    NimBLEServer* server = NimBLEDevice::getServer();
    if (server != nullptr) {
      uint8_t connectedCount = server->getConnectedCount();
      primaryBlePeerConnected = connectedCount > 0;
      NimBLEAdvertising* advertising = server->getAdvertising();
      if (advertising != nullptr
          && connectedCount > 0
          && connectedCount != state.lastGattConnectedCount
          && !advertising->isAdvertising()) {
        server->startAdvertising();
      }
      state.lastGattConnectedCount = connectedCount;
    }
  }
}

void secondary_tick(LinkState& state) {
  // If we're the secondary, and disconnected, try to connect to the primary.
  if (state.allowGatt && !state.isConnected) {
    connect_to_primary_gatt();
  }
}

void tick(LinkState& state) {
  if (boardConfig.isPrimary) {
    primary_tick(state);
  } else {
    secondary_tick(state);
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
      case KeymapResolver::ActionType::MediaTap:
        gatt_send_media_key(action.mediaCode);
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
      case KeymapResolver::ActionType::MediaRelease:
        return false;
    }
  }

  return false;
}

}
