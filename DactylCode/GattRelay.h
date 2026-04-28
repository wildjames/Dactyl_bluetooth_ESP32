// GattRelay.h
//
// Wireless inter-half communication via a custom BLE GATT service.
//
// At boot, both halves attempt a Serial2 handshake to decide which transport to use:
//   - Cable detected  → split_keeb_communication = true  (existing Serial2 path)
//   - No cable        → use_gatt = true
//
// In GATT mode:
//   Master  – adds a custom relay service to the NimBLE server that bleKB already owns.
//             The slave connects and writes key events; the master forwards them via bleKB.
//   Slave   – does NOT call bleKB.begin().  Instead it initialises NimBLE as a central,
//             scans for the master's relay service, and writes key events to it.
//
// Requires NimBLE-Arduino 1.4 or later (NimBLEConnInfo parameter in callbacks).

#pragma once

#include <NimBLEDevice.h>

// ── UUIDs ─────────────────────────────────────────────────────────────────────
#define RELAY_SERVICE_UUID  "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#define KEY_EVENT_CHAR_UUID "BEB5483E-36E1-4688-B7F5-EA07361B26A8"

// ── GATT packet format: [event_type, byte1, byte2] ───────────────────────────
//   Regular key press:   GATT_KEY_PRESS,   keycode,          modifier
//   Regular key release: GATT_KEY_RELEASE, keycode,          modifier
//   Media key tap:       GATT_MEDIA_KEY,   mediacode_low,    mediacode_high
#define GATT_KEY_PRESS   0x01
#define GATT_KEY_RELEASE 0x00
#define GATT_MEDIA_KEY   0x02

// ── Boot-time wire-detection ──────────────────────────────────────────────────
#define WIRE_DETECT_SYNC          0xAA
#define WIRE_DETECT_ACK           0x55
#define WIRE_DETECT_TIMEOUT_MS    2000  // total window (ms)
#define WIRE_DETECT_PING_INTERVAL 50    // ms between master SYNC pings

// ── Slave scans for the master by BLE device name ────────────────────────────
// BoardConfig_R.h defines MASTER_BLE_NAME; provide a fallback for the master
// half (which never uses this define but must compile without it).
#ifndef MASTER_BLE_NAME
#define MASTER_BLE_NAME "TwoBrownFoxes"
#endif

// Forward-declare globals defined later in DactylCode.ino (same compilation unit).
extern bool is_connected;

// ─────────────────────────────────────────────────────────────────────────────
// Boot-time detection
// ─────────────────────────────────────────────────────────────────────────────
// Opens Serial2 and exchanges a SYNC/ACK handshake.
// Returns true  → halves are wired; caller should set split_keeb_communication=true.
// Returns false → halves are not wired; caller should set use_gatt=true.
// Serial2 is left open so the wired path can use it immediately.
bool detect_wired_connection() {
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);

  unsigned long deadline = millis() + WIRE_DETECT_TIMEOUT_MS;

  if (is_master) {
    // Master: ping SYNC repeatedly; return true the moment it gets ACK.
    while (millis() < deadline) {
      Serial2.write((uint8_t)WIRE_DETECT_SYNC);
      unsigned long ping_end = millis() + WIRE_DETECT_PING_INTERVAL;
      while (millis() < ping_end) {
        if (Serial2.available() && Serial2.read() == WIRE_DETECT_ACK) {
          if (DEBUG) { Serial.println("[DETECT] Wired: ACK received"); }
          return true;
        }
      }
    }
    if (DEBUG) { Serial.println("[DETECT] No ACK — going wireless"); }
    return false;

  } else {
    // Slave: wait for SYNC, reply with ACK, return true.
    while (millis() < deadline) {
      if (Serial2.available() && Serial2.read() == WIRE_DETECT_SYNC) {
        Serial2.write((uint8_t)WIRE_DETECT_ACK);
        if (DEBUG) { Serial.println("[DETECT] Wired: SYNC received"); }
        return true;
      }
    }
    if (DEBUG) { Serial.println("[DETECT] No SYNC — going wireless"); }
    return false;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Master: GATT relay server
// ─────────────────────────────────────────────────────────────────────────────
// Callbacks for key-event writes from the slave.
class KeyEventCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
    std::string val = pChar->getValue();
    if (val.length() < 2) return;

    uint8_t evt     = (uint8_t)val[0];
    uint8_t b1      = (uint8_t)val[1];
    uint8_t b2      = (val.length() >= 3) ? (uint8_t)val[2] : 0;

    if (DUMMY) return;

    if (evt == GATT_KEY_PRESS) {
      bleKB.press(b1, b2);              // b1 = keycode, b2 = modifier
    } else if (evt == GATT_KEY_RELEASE) {
      bleKB.release(b1);                // b1 = keycode
      if (b2) bleKB.release(KEY_LSHIFT); // b2 = modifier; release shift if needed
    } else if (evt == GATT_MEDIA_KEY) {
      uint16_t mediaCode = (uint16_t)b1 | ((uint16_t)b2 << 8);
      bleKB.tap(mediaCode);
    }
  }
};

// Callbacks for slave connect/disconnect events.
// NOTE: We do NOT install server callbacks here because bleKB already owns them.
//       The is_connected flag is updated via the GattServerCallbacks removed
//       intentionally — instead the master monitors pairing via bleKB's own
//       callbacks and the slave sets is_connected itself after connecting.

// Call once during setup(), AFTER bleKB.begin(), when use_gatt && is_master.
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
      scanResponse.setName(MASTER_BLE_NAME);
      pAdv->setScanResponseData(scanResponse);
      pAdv->start();
    }
  }

  if (DEBUG) { Serial.println("[GATT] Relay server started"); }
}

// ─────────────────────────────────────────────────────────────────────────────
// Slave: GATT relay client (central role)
// ─────────────────────────────────────────────────────────────────────────────
static NimBLERemoteCharacteristic* pRemoteKeyChar = nullptr;
static bool gatt_client_ready = false;

class GattClientCallbacks : public NimBLEClientCallbacks {
  void onDisconnect(NimBLEClient* pClient, int reason) override {
    if (DEBUG) { Serial.println("[GATT] Disconnected from master"); }
    pRemoteKeyChar     = nullptr;
    gatt_client_ready = false;
    is_connected      = false;
  }
};

// Call once during setup() when use_gatt && !is_master.
// bleKB.begin() must NOT have been called; this function initialises NimBLE
// directly as a central (no HID advertising).
// Blocks until the master is found and connected (retries indefinitely).
// Returns true once connected; never returns false.
bool connect_to_master_gatt() {
  if (!NimBLEDevice::isInitialized()) {
    NimBLEDevice::init("");         // Init BLE without HID
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  }

  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setActiveScan(true);
  pScan->setInterval(45);
  pScan->setWindow(15);

  while (true) {
    if (DEBUG) { Serial.println("[GATT] Scanning for master relay service..."); }

    // start() duration is in milliseconds in NimBLE 2.x (not seconds).
    // Non-blocking; wait on isScanning() for it to finish.
    pScan->start(10000, false);
    while (pScan->isScanning()) { delay(100); }

    NimBLEScanResults results = pScan->getResults();

    if (DEBUG) {
      Serial.print("[GATT] Scan complete, devices found: ");
      Serial.println(results.getCount());
    }

    bool retry = true;
    for (int i = 0; i < results.getCount(); i++) {
      const NimBLEAdvertisedDevice* dev = results.getDevice(i);
      // Match on device name — the 128-bit relay UUID is too large to fit in
      // the HID advertisement packet so UUID-based filtering is unreliable.
      if (dev->getName() != MASTER_BLE_NAME) {
        continue;
      }

      if (DEBUG) {
        Serial.print("[GATT] Found master: ");
        Serial.println(dev->getName().c_str());
      }

      NimBLEClient* pClient = NimBLEDevice::createClient();
      pClient->setClientCallbacks(new GattClientCallbacks(), false);
      pClient->setConnectionParams(12, 12, 0, 51);
      pClient->setConnectTimeout(5000);

      if (!pClient->connect(dev)) {
        if (DEBUG) { Serial.println("[GATT] Connection failed; will retry scan"); }
        NimBLEDevice::deleteClient(pClient);
        break; // break inner for-loop; outer while retries
      }

      NimBLERemoteService* pSvc = pClient->getService(RELAY_SERVICE_UUID);
      if (!pSvc) {
        if (DEBUG) { Serial.println("[GATT] Relay service not found on master; retrying"); }
        pClient->disconnect();
        NimBLEDevice::deleteClient(pClient);
        break;
      }

      pRemoteKeyChar = pSvc->getCharacteristic(KEY_EVENT_CHAR_UUID);
      if (!pRemoteKeyChar) {
        if (DEBUG) { Serial.println("[GATT] Key characteristic not found; retrying"); }
        pClient->disconnect();
        NimBLEDevice::deleteClient(pClient);
        break;
      }

      gatt_client_ready = true;
      is_connected      = true;
      if (DEBUG) { Serial.println("[GATT] Connected to master relay server"); }
      return true;
    }

    if (DEBUG) { Serial.println("[GATT] Master not found, retrying scan..."); }
    pScan->clearResults();
    delay(1000);
  }
}

// ── Send helpers called from send_keypress() ──────────────────────────────────
void gatt_send_key_press(uint8_t keycode, uint8_t modifier) {
  if (!gatt_client_ready || !pRemoteKeyChar) return;
  uint8_t data[3] = { GATT_KEY_PRESS, keycode, modifier };
  pRemoteKeyChar->writeValue(data, 3, false);
}

void gatt_send_key_release(uint8_t keycode, uint8_t modifier) {
  if (!gatt_client_ready || !pRemoteKeyChar) return;
  uint8_t data[3] = { GATT_KEY_RELEASE, keycode, modifier };
  pRemoteKeyChar->writeValue(data, 3, false);
}

void gatt_send_media_key(uint16_t mediaCode) {
  if (!gatt_client_ready || !pRemoteKeyChar) return;
  uint8_t data[3] = { GATT_MEDIA_KEY,
                      (uint8_t)(mediaCode & 0xFF),
                      (uint8_t)(mediaCode >> 8) };
  pRemoteKeyChar->writeValue(data, 3, false);
}
