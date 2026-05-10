# DactylCode

This sketch is the firmware for my bluetooth split keyboard. The architecture is fairly simple, there is an overarching loop in the main sketch, and a load of handlers and resolvers which are each responsible for one thing (in theory). I've tried to have as little crossover as possible, and to use hooks to those handlers so that extensions should be relatively easy to make.

Some amount of configuration-passing is inevitable, but again I've done my best to make that reasonably ambivilent to changes in other modules. Where some state needs to be persisted, I've kept that on the `RuntimeState` module. I suppose I could have a `PowerState.h`, `LEDState.h` etc, and include them all in a central `RuntimeState` to make this even MORE modular, but honestly I think the current solution is good enough for this scale.

As a reminder,

| File | Purpose |
|------|---------|
| `DactylCode` | Main sketch - setup, loop, and action dispatch |
| `BoardConfig` | Shared struct definitions for board configuration |
| `MatrixScanner` | Scans the key matrix and tracks press/release state |
| `KeymapResolver` | Resolves pressed keys into actions (layers, toggles, etc.) |
| `HidDispatcher` | Sends resolved actions as HID key events via BLE |
| `LinkManager` | Manages the inter-board BLE GATT connection |
| `GattRelay` | Custom GATT service for wireless event relay |
| `PowerManager` | Battery monitoring and deep sleep |
| `StatusLed` | Status LED control (connected/disconnected indication) |
| `RuntimeState` | Runtime state structs (link, battery, matrix, LED, loop) |


## Deep Sleep logic

To save power, after a period of inactivity the boards can be configured to enter deep sleep. The boards are not physically connected, but they do form two halves of a whole and this makes the decision to go to sleep slightly harder - each half should ideally account for activity from it's partner before sleeping. To do that, there is also some GATT characteristics to communicate sleep activity and try and sync this between them. This flow diagram details the deep sleep thought process:

```mermaid
flowchart TD
    A["loop() ends"] --> B["sleep_if_idle()"]
    B --> C{"is_battery_charging()?"}
    C -- "Yes (charge rate > 0<br/>or USB connected)" --> Z["Return - stay awake"]
    C -- No --> D{"millis() - lastActivity<br/>> deepSleepWaitMs?"}
    D -- No --> Z
    D -- Yes --> E["PowerManager::enter_deep_sleep()"]

    E --> E1["PowerManager sets up the deep sleep state - calls relevant functions from other modules"]
    E1 --> E2["esp_sleep_enable_ext1_wakeup()"]
    E2 --> E3["esp_deep_sleep_start()"]

    E3 -.->|"key pressed"| W["ESP32 wakes, runs setup()"]
    W --> W1["release_sleep_matrix_config()"]
    W1 --> W2["Normal boot continues<br/>(BLE, matrix, timers)"]

    subgraph "lastActivity reset sources"
        direction TB
        R1["Local keypresses<br/>(DactylCode.ino loop)"]
        R2["Secondary → Primary<br/>GATT key events<br/>(GattRelay onWrite)"]
        R3["Primary → Secondary<br/>activity notification<br/>(GATT subscribe callback)"]
    end

    R1 -->|"resets"| LA["runtimeState.loop.lastActivity"]
    R2 -->|"resets"| LA
    R3 -->|"resets"| LA
    LA -->|"compared in"| D
```


## GATT Relay

The GATT relay is the method by which the two halves communicate wirelessly. The primary half hosts a custom GATT service alongside the BLE HID keyboard service that talks to the PC. The secondary half connects as a client to this relay service and writes its key events into it. From there, the primary forwards them to the host as if they were its own keypresses. The flow of data is bi-directional - the secondary writes key events and battery levels up to the primary, and the primary pushes activity notifications back down to the secondary (so it knows not to fall asleep while the other half is in use).

### The service

The relay lives on a single GATT service (`RELAY_SERVICE_UUID`) with two characteristics:

| Characteristic | Direction | Purpose |
|----------------|-----------|---------|
| `KEY_EVENT_CHAR` | Secondary -> Primary | Key press/release/tap events and battery level updates. The secondary writes a short packet here and the primary's `onWrite` callback dispatches it to `HidDispatcher`. |
| `ACTIVITY_NOTIFY_CHAR` | Primary -> Secondary | Activity pings. The primary notifies this characteristic whenever it has local keypresses, so the secondary resets its sleep timer. Throttled to avoid BLE congestion. |

The packet format is pretty minimal - a one-byte event type followed by one or two payload bytes:

| Event | Byte 0 | Byte 1 | Byte 2 |
|-------|--------|--------|--------|
| Key press | `0x01` | keycode | - |
| Key release | `0x00` | keycode | - |
| Key tap | `0x02` | keycode low | keycode high |
| Battery level | `0x03` | percentage | - |
| Activity ping | `0x04` | - | - |

### Primary vs secondary roles

The primary half doesn't create its own NimBLE server - `bleKB.begin()` already does that as part of the HID setup. Instead, `setup_gatt_server()` grabs the existing server and bolts the relay service onto it. This means advertising and server callbacks are entirely owned by bleKB, and the relay just piggybacks. It's a bit of a dance, but it avoids the crashes I was getting from trying to manage advertising in two places.

The secondary half never calls `bleKB.begin()` at all, since it relies on the primary to send its keystrokes. It initialises NimBLE as a central, scans for the primary's name, connects, and grabs a handle to the key event characteristic. If the connection drops, `LinkManager` retries on its next tick. The secondary also subscribes to the activity notification characteristic so that keypresses on the primary half will keep it awake.

### USB fallback

One slightly tricky bit: when the primary is plugged into USB, it switches its HID output to USB but keeps the GATT relay alive. The secondary can still forward keystrokes over BLE, and the primary sends them out over the USB connection instead. If the _secondary_ gets plugged into USB, it disconnects from the primary entirely and acts as its own USB HID device - no relay needed.

#### Primary half (server)

```mermaid
flowchart TD
    P1["LinkManager::begin()"] --> P2["HidDispatcher::begin()<br/>(bleKB.begin - creates NimBLE server)"]
    P2 --> P3["setup_gatt_server()"]
    P3 --> P4["Relay service live:"]

    P4 --> PT["primary_tick() - each loop"]
    PT --> PT1{"USB just plugged in?"}
    PT1 -- Yes --> PT2["Switch HID output to USB,<br/>keep GATT relay alive"]
    PT1 -- No --> PT3{"USB just unplugged?"}
    PT3 -- Yes --> PT4["Resume BLE HID output"]
    PT3 -- No --> PT5["Maintain GATT advertising<br/>(restart if peer count changed)"]
```

#### Secondary half (client)

```mermaid
flowchart TD
    S1["LinkManager::begin()"] --> S2["connect_to_primary_gatt()"]
    S2 --> S3["Init NimBLE as central<br/>(no bleKB, no HID)"]
    S3 --> S4["Scan for primary by name"]
    S4 --> S5{"Found?"}
    S5 -- No --> S6["Return false -<br/>retry next tick"]
    S5 -- Yes --> S7["Connect, discover<br/>relay service"]
    S7 --> S8["Grab KEY_EVENT_CHAR handle"]
    S8 --> S9["Subscribe to<br/>ACTIVITY_NOTIFY_CHAR"]
    S9 --> S10["Client ready -<br/>key events flow"]

    S10 --> ST["secondary_tick()"]
    ST --> ST1{"USB just plugged in?"}
    ST1 -- Yes --> ST2["Disconnect GATT,<br/>switch to USB HID"]
    ST1 -- No --> ST3{"Connection dropped?"}
    ST3 -- Yes --> S2
    ST3 -- No --> ST4["Continue sending<br/>key events via GATT"]
```

#### Data flow (steady state)

```mermaid
flowchart LR
    KE["Secondary keypress"] -->|"gatt_send_key_press()"| WR["Write to KEY_EVENT_CHAR"]
    WR -->|"onWrite callback"| FW["Primary dispatches<br/>via HidDispatcher"]
    FW --> HOST["Host PC"]

    PK["Primary keypress"] -->|"gatt_notify_activity()"| AN["Notify ACTIVITY_NOTIFY_CHAR"]
    AN -->|"subscribe callback"| RST["Secondary resets<br/>lastActivity timer"]

    BAT["Secondary battery update"] -->|"gatt_send_battery_level()"| WR2["Write to KEY_EVENT_CHAR"]
    WR2 -->|"onWrite callback"| BS["Primary stores<br/>companionPercentage"]
```
