// GattRelay.h
//
// Wireless inter-half communication via a custom BLE GATT service.
//
// The halves always use GATT mode:
//   Primary   - adds a custom relay service to the NimBLE server that bleKB already owns.
//               The secondary half connects and writes key events; the primary forwards them via bleKB.
//   Secondary - does NOT call bleKB.begin(). Instead it initialises NimBLE as a central,
//               scans for the primary half's relay service, and writes key events to it.
//
// Requires NimBLE-Arduino 1.4 or later (NimBLEConnInfo parameter in callbacks).

#pragma once

#include <NimBLEDevice.h>

#include "RuntimeState.h"
#include "HidDispatcher.h"

// ── UUIDs ─────────────────────────────────────────────────────────────────────
#define RELAY_SERVICE_UUID  "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#define KEY_EVENT_CHAR_UUID "BEB5483E-36E1-4688-B7F5-EA07361B26A8"

// ── GATT packet format: [event_type, byte1, (byte2)] ────────────────────────
//   Regular key press:   GATT_KEY_PRESS,   keycode
//   Regular key release: GATT_KEY_RELEASE, keycode
//   Media key tap:       GATT_MEDIA_KEY,   mediacode_low,    mediacode_high
//   Battery update:      GATT_BATTERY_LEVEL, percentage_or_0xFF
#define GATT_KEY_PRESS   0x01
#define GATT_KEY_RELEASE 0x00
#define GATT_MEDIA_KEY   0x02
#define GATT_BATTERY_LEVEL 0x03

#define GATT_SCAN_BURST_MS        750   // ms per discovery pass before attempting connect
#define GATT_RETRY_DELAY_MS       100   // ms between scan retries when nothing is found
#define GATT_CONNECT_TIMEOUT_MS   2000  // ms before abandoning a connection attempt

// Shared runtime link state defined in DactylCode.ino.
extern LinkState& linkState;

// ─────────────────────────────────────────────────────────────────────────────
// Primary half: GATT relay server
// ─────────────────────────────────────────────────────────────────────────────
// Callbacks for key-event writes from the secondary half.
class KeyEventCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
    std::string val = pChar->getValue();
    if (val.length() < 2) return;

    uint8_t evt     = (uint8_t)val[0];
    uint8_t b1      = (uint8_t)val[1];
    uint8_t b2      = (val.length() >= 3) ? (uint8_t)val[2] : 0;

    if (boardConfig.dummy) return;

    if (evt == GATT_KEY_PRESS) {
      HidDispatcher::press_key(b1, boardConfig.dummy);
    } else if (evt == GATT_KEY_RELEASE) {
      HidDispatcher::release_key(b1, boardConfig.dummy);
    } else if (evt == GATT_MEDIA_KEY) {
      uint16_t mediaCode = (uint16_t)b1 | ((uint16_t)b2 << 8);
      HidDispatcher::tap_media(mediaCode, boardConfig.dummy);
    }
  }
};

// Connection state is tracked from the secondary-half relay link.
// NOTE: We do NOT install server callbacks here because bleKB already owns them.
//       The is_connected flag is updated via the GattServerCallbacks removed
//       intentionally - instead the primary half monitors pairing via bleKB's own
//       callbacks and the secondary half sets is_connected itself after connecting.

// Call once during setup(), AFTER bleKB.begin(), when use_gatt && is_primary.
// bleKB.begin() has already initialised the NimBLE stack; we retrieve the
// existing server and attach the relay service to it.
// We do NOT touch advertising or server callbacks here — bleKB owns those.
void setup_gatt_server() {
  NimBLEServer* pServer = NimBLEDevice::getServer();
  if (pServer == nullptr) {
    pServer = NimBLEDevice::createServer();
  }
  // Do NOT call pServer->setCallbacks() — bleKB already installed its own.

  NimBLEService* pSvc = pServer->getServiceByUUID(RELAY_SERVICE_UUID);
  if (pSvc == nullptr) {
    pSvc = pServer->createService(RELAY_SERVICE_UUID);

    NimBLECharacteristic* pChar = pSvc->createCharacteristic(
      KEY_EVENT_CHAR_UUID,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pChar->setCallbacks(new KeyEventCallbacks());

    // NimBLE 2.x starts services when the server starts; calling
    // NimBLEService::start() here is a no-op and does not register the new
    // service in the live GATT database.
    pServer->start();

    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    if (pAdv != nullptr) {
      NimBLEAdvertisementData scanResponse;
      scanResponse.setName(boardConfig.primaryBleName);
      pAdv->setScanResponseData(scanResponse);
      pAdv->start();
    }
  }

  if (boardConfig.debug) { Serial.println("[GATT] Relay server started"); }
}

// ─────────────────────────────────────────────────────────────────────────────
// Secondary half: GATT relay client (central role)
// ─────────────────────────────────────────────────────────────────────────────
static NimBLERemoteCharacteristic* pRemoteKeyChar = nullptr;
static bool gatt_client_ready = false;

class GattClientCallbacks : public NimBLEClientCallbacks {
  void onDisconnect(NimBLEClient* pClient, int reason) override {
    if (boardConfig.debug) { Serial.println("[GATT] Disconnected from primary half"); }
    pRemoteKeyChar     = nullptr;
    gatt_client_ready = false;
    linkState.isConnected = false;
  }
};

// Call once during setup() when use_gatt && !is_primary.
// bleKB.begin() must NOT have been called; this function initialises NimBLE
// directly as a central (no HID advertising).
// Blocks until the primary half is found and connected (retries indefinitely).
// Returns true once connected; never returns false.
bool connect_to_primary_gatt() {
  if (!NimBLEDevice::isInitialized()) {
    NimBLEDevice::init("");         // Init BLE without HID
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  }

  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setActiveScan(true);
  pScan->setInterval(30);
  pScan->setWindow(30);

  while (true) {
    if (boardConfig.debug) { Serial.println("[GATT] Scanning for primary relay service..."); }

    // Scan in short bursts so we can attempt a connection as soon as the
    // primary half is discovered instead of waiting for a long scan to finish.
    pScan->start(GATT_SCAN_BURST_MS, false);
    while (pScan->isScanning()) { delay(100); }

    NimBLEScanResults results = pScan->getResults();

    if (boardConfig.debug) {
      Serial.print("[GATT] Scan complete, devices found: ");
      Serial.println(results.getCount());
    }

    for (int i = 0; i < results.getCount(); i++) {
      const NimBLEAdvertisedDevice* dev = results.getDevice(i);
      // Match on device name — the 128-bit relay UUID is too large to fit in
      // the HID advertisement packet so UUID-based filtering is unreliable.
      if (dev->getName() != boardConfig.primaryBleName) {
        continue;
      }

      if (boardConfig.debug) {
        Serial.print("[GATT] Found primary half: ");
        Serial.println(dev->getName().c_str());
      }

      NimBLEClient* pClient = NimBLEDevice::createClient();
      pClient->setClientCallbacks(new GattClientCallbacks(), false);
      pClient->setConnectionParams(12, 12, 0, 51);
      pClient->setConnectTimeout(GATT_CONNECT_TIMEOUT_MS);

      if (!pClient->connect(dev)) {
        if (boardConfig.debug) { Serial.println("[GATT] Connection failed; will retry scan"); }
        NimBLEDevice::deleteClient(pClient);
        break; // break inner for-loop; outer while retries
      }

      NimBLERemoteService* pSvc = pClient->getService(RELAY_SERVICE_UUID);
      if (!pSvc) {
        if (boardConfig.debug) { Serial.println("[GATT] Relay service not found on primary half; retrying"); }
        pClient->disconnect();
        NimBLEDevice::deleteClient(pClient);
        break;
      }

      pRemoteKeyChar = pSvc->getCharacteristic(KEY_EVENT_CHAR_UUID);
      if (!pRemoteKeyChar) {
        if (boardConfig.debug) { Serial.println("[GATT] Key characteristic not found; retrying"); }
        pClient->disconnect();
        NimBLEDevice::deleteClient(pClient);
        break;
      }

      gatt_client_ready = true;
      linkState.isConnected = true;
      if (boardConfig.debug) { Serial.println("[GATT] Connected to primary relay server"); }
      return true;
    }

    if (boardConfig.debug) { Serial.println("[GATT] Primary half not found, retrying scan..."); }
    pScan->clearResults();
    delay(GATT_RETRY_DELAY_MS);
  }
}

// ── Send helpers called from the key event dispatch path ─────────────────────
void gatt_send_key_press(uint8_t keycode) {
  if (!gatt_client_ready || !pRemoteKeyChar) return;
  uint8_t data[2] = { GATT_KEY_PRESS, keycode };
  pRemoteKeyChar->writeValue(data, 2, false);
}

void gatt_send_key_release(uint8_t keycode) {
  if (!gatt_client_ready || !pRemoteKeyChar) return;
  uint8_t data[2] = { GATT_KEY_RELEASE, keycode };
  pRemoteKeyChar->writeValue(data, 2, false);
}

void gatt_send_media_key(uint16_t mediaCode) {
  if (!gatt_client_ready || !pRemoteKeyChar) return;
  uint8_t data[3] = { GATT_MEDIA_KEY,
                      (uint8_t)(mediaCode & 0xFF),
                      (uint8_t)(mediaCode >> 8) };
  pRemoteKeyChar->writeValue(data, 3, false);
}
