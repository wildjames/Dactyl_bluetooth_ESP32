#include "LinkManager.h"

#include "BoardConfig.h"
#include "GattRelay.h"
#include "HidDispatcher.h"
#include "MatrixScanner.h"

namespace {

constexpr uint8_t PRESS_FLAG = B00001111;
constexpr uint8_t RELEASE_FLAG = B11110000;

bool primaryBlePeerConnected = false;

void send_keep_alive(LinkState& state) {
  if (!boardConfig.isPrimary) {
    return;
  }

  if (boardConfig.debug) {
    Serial.println("Sending keep alive...");
  }

  Serial2.write(state.keepAliveMessage);
  state.lastKeepAliveCheck = millis();
}

void poll_wired_serial(LinkState& state, bool dummy) {
  bool continueParsing = true;
  while (continueParsing) {
    if (Serial2.available()) {
      if (boardConfig.debug) {
        Serial.println("I have serial to parse...");
      }

      int message = Serial2.read();
      if (message == state.keepAliveMessage) {
        if (boardConfig.debug) {
          Serial.println("Got a keep alive message! Still linked to the other half.");
        }
        state.isConnected = true;
        state.lastKeepAliveTime = millis();
      } else if (message == PRESS_FLAG || message == RELEASE_FLAG) {
        if (boardConfig.debug) {
          Serial.print("I got the message ");
          Serial.print(message);
          Serial.println(" so I expect a second message. Waiting for that now...");
        }

        while (!Serial2.available()) {
        }

        uint8_t received = (uint8_t)Serial2.read();
        if (message == PRESS_FLAG) {
          HidDispatcher::press_passthrough(received, dummy);
        } else {
          HidDispatcher::release_passthrough(received);
        }
      } else if (boardConfig.debug) {
        Serial.println(message);
      }
    }

    continueParsing = Serial2.available() > 0;
  }
}

}

namespace LinkManager {

void begin(LinkState& state) {
  if (boardConfig.enableSerialSplit && detect_wired_connection()) {
    state.splitCommunication = true;
    state.useGatt = false;
  } else {
    state.splitCommunication = false;
    state.useGatt = true;
    if (boardConfig.enableSerialSplit) {
      Serial2.end();
      MatrixScanner::configure_pins(boardConfig);
    }
  }

  if (boardConfig.debug) {
    Serial.println(state.splitCommunication ? "Connection: WIRED (Serial2)"
                                            : "Connection: WIRELESS (GATT)");
  }

  // Match the pre-refactor init order: first choose transport, then start the
  // local HID/NimBLE stack for every mode except the wireless secondary.
  if (!(state.useGatt && !boardConfig.isPrimary)) {
    HidDispatcher::begin();
  }

  if (!state.useGatt) {
    return;
  }

  if (boardConfig.isPrimary) {
    setup_gatt_server();
  } else {
    connect_to_primary_gatt();
  }
}

void tick(LinkState& state) {
  bool isWirelessSecondary = state.useGatt && !boardConfig.isPrimary;
  if (isWirelessSecondary && !state.isConnected) {
    connect_to_primary_gatt();
  }

  primaryBlePeerConnected = false;
  if (state.useGatt && boardConfig.isPrimary) {
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

  if (state.splitCommunication) {
    if (millis() - state.lastKeepAliveCheck > boardConfig.timings.keepAliveDelayMs) {
      send_keep_alive(state);
    }
    if (millis() - boardConfig.timings.keepAliveLifespanMs > state.lastKeepAliveTime) {
      state.isConnected = false;
    }
  }
}

void poll_incoming(LinkState& state, bool dummy) {
  if (!state.splitCommunication) {
    return;
  }

  poll_wired_serial(state, dummy);
}

bool has_primary_ble_peer() {
  return primaryBlePeerConnected;
}

bool dispatch_remote_action(const KeymapResolver::Action& action, const LinkState& state) {
  if (state.useGatt) {
    switch (action.type) {
      case KeymapResolver::ActionType::MediaTap:
        gatt_send_media_key(action.mediaCode);
        return true;

      case KeymapResolver::ActionType::KeyPress:
        gatt_send_key_press(action.keycode, action.modifier);
        return true;

      case KeymapResolver::ActionType::KeyRelease:
        gatt_send_key_release(action.keycode, action.modifier);
        return true;

      case KeymapResolver::ActionType::None:
      case KeymapResolver::ActionType::ReleaseAll:
      case KeymapResolver::ActionType::TapCapsLock:
      case KeymapResolver::ActionType::MediaRelease:
        return false;
    }
  }

  if (!state.splitCommunication) {
    return false;
  }

  switch (action.type) {
    case KeymapResolver::ActionType::KeyPress:
      Serial2.write(PRESS_FLAG);
      Serial2.write(action.keycode);
      return true;

    case KeymapResolver::ActionType::KeyRelease:
      Serial2.write(RELEASE_FLAG);
      Serial2.print(action.keycode);
      return true;

    case KeymapResolver::ActionType::None:
    case KeymapResolver::ActionType::ReleaseAll:
    case KeymapResolver::ActionType::TapCapsLock:
    case KeymapResolver::ActionType::MediaTap:
    case KeymapResolver::ActionType::MediaRelease:
      return false;
  }

  return false;
}

}
