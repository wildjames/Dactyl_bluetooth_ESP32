# Dactyl Manuform with Bluetooth via ESP32

The dactyl manuform keyboard, with bluetooth. Fully wireless, both halves.

This is by no means a polished, easy-to-replicate project. QMK didn't properly support the ESP32's bluetooth, and their recommended alternative of a pro micro with a bluetooth breakout board doesn't sound easier to me. The Arduino code in this repo started as a stop-gap until QMK caught up, but honestly it's grown into something fairly capable at this point, so here we are.

The [Dactyl Manuform](https://github.com/abstracthat/dactyl-manuform) is a great split keyboard with a pretty non-standard form. It's a split keyboard — typically the two halves have their own microcontrollers communicating over TRRS or similar. I wanted a wireless solution, and preferably one that's a bit more flexible. This is the result, built on top of the excellent [HijelHID_BLEKeyboard](https://github.com/wildjames/HijelHID_BLEKeyboard) library for ESP32 HID-over-BLE.

If you've seen the dactyl before and you're on the fence, I'd say pull the trigger. Learning the layout is worth the trouble, and with some nice clicky blues this is a great keyboard.

**Note:** You'll need [this fork](https://github.com/wildjames/HijelHID_BLEKeyboard/tree/multiple-clients) of the BLE keyboard library, which patches a bug where the host half would drop its PC connection when the secondary half disconnected.

## How it works

Each half runs an ESP32. The **primary** half (left, by default) connects to your PC as a BLE HID keyboard. The **secondary** half connects to the primary over a custom BLE GATT service and forwards its key events wirelessly. The primary then relays those keystrokes to the PC alongside its own — from the PC's perspective, it's just one keyboard.

I've repurposed the physical cable between the halves purely for sharing charging current. Plug one half in and the other charges too. Keystrokes are **never** sent over USB.

### Features

- **Layers:** A modifier key gives access to a second layer (function keys, navigation, etc.). I left it at one extra layer because I can never remember more than that.
- **Alternate layout toggle:** A configurable key can toggle the entire base layout, e.g. QWERTY/Dvorak switch via double-tap.
- **Media keys:** Full media key support — play/pause, volume, brightness, the works.
- **Double-tap caps lock:** Double-tap the shift key (configurable) to toggle caps lock.
- **Deep sleep:** Both halves enter deep sleep after configurable idle time. Mash keys to wake them up.
- **Battery monitoring:** Battery level is reported to the host and periodically updated.

### Hardware

I initially used plain old ESP32 dev boards, but the cheap breakout battery circuit I had drew far too much current even when the controller was asleep. I caved and used Adafruit Feather HUZZAH32 boards instead. If you're making this project, I'd recommend the same. It's not the best deep sleep current draw out there, but off my 4000mAh batteries I get standby times in the ~month range. Plus, Adafruit sacrifice an analog pin for battery monitoring with no extra work on your behalf, so that's trivial.

I used RJ9 connectors to join the halves (for the shared charging current) and USB-C to supply power to the controllers.

I'm moving to these boards: https://learn.adafruit.com/adafruit-esp32-s3-feather/pinouts

## Code structure

The firmware is in `DactylCode/` and is split into modules:

| File | Purpose |
|------|---------|
| `DactylCode.ino` | Main sketch — setup, loop, and action dispatch |
| `BoardConfig.h` | Shared struct definitions for board configuration |
| `config/BoardConfig_L.h` | Left half pin assignments, timings, and settings |
| `config/BoardConfig_R.h` | Right half pin assignments, timings, and settings |
| `config/KeyLayout_L.h` | Left half keymap (layer indices) |
| `config/KeyLayout_R.h` | Right half keymap (layer indices) |
| `MatrixScanner.h/.cpp` | Scans the key matrix and tracks press/release state |
| `KeymapResolver.h/.cpp` | Resolves pressed keys into actions (layers, toggles, etc.) |
| `HidDispatcher.h/.cpp` | Sends resolved actions as HID key events via BLE |
| `LinkManager.h/.cpp` | Manages the inter-half BLE GATT connection |
| `GattRelay.h` | Custom GATT service for wireless key event relay |
| `PowerManager.h/.cpp` | Battery monitoring and deep sleep |
| `StatusLed.h/.cpp` | Status LED control (connected/disconnected indication) |
| `RuntimeState.h` | Runtime state structs (link, battery, matrix, LED, loop) |

To switch between compiling for the left or right half, comment/uncomment the relevant `#include` at the top of `DactylCode.ino`:

```cpp
#include "config/BoardConfig_L.h"
// #include "config/BoardConfig_R.h"
```

## Tailoring the code for yourself

Each half has its own config file under `config/` — `BoardConfig_L.h` and `BoardConfig_R.h`. These contain all the user-editable stuff you should need: pin assignments, timing parameters, LED config, battery thresholds, and whether the half is primary or secondary. I hope most of it is self-explanatory.

### Making keymaps

I designed this for a 5x6 manuform, requiring 5 columns + 7 rows of GPIO (5x6 for the main keys, plus one extra row for the thumb cluster). The code should work fine with other layouts — just update `MATRIX_KEY_COUNT` in `BoardConfig.h`.

The keymap system works like this: your per-half `KeyLayout_L.h` or `KeyLayout_R.h` then defines a `keymap[]` array where each entry is the keystroke for that index in the key matrix. Layers are stacked — layer 0 occupies `keymap[0..NKEYS-1]`, layer 1 occupies `keymap[NKEYS..2*NKEYS-1]`, and so on.

I won't pretend this is fun to set up by hand. The easiest (but slow) way is to enable `debug` and `dummy` in your board config, then press each key to see its matrix index printed over serial. Fill in the keymap from there.

There's also a `Dactyl_keymapper/` sketch that will prompt you for each key and build a keymap. It'll only cover the keys it asks about though, so no media keys or other exotic stuff. `Dactyl_keyFinder` will print the key index for any keys you hit, which can be helpful as well.

### The LED

I didn't want full per-key illumination because of power consumption, but a status LED is still nice. I wired one underneath the escape/backspace area on each half. It flashes when the keyboard is searching for a connection and stays solid when paired. Configuration is in the `led` section of your board config (pin, PWM frequency, resolution, max duty cycle).

### Deep sleep and waking

After a configurable idle period, both halves enter deep sleep to save power. Wake-up is triggered by specific RTC-capable pins going high — you define which column pins to use as wake sources in `WAKE_PINS[]` in your board config.

Fair warning: on my Feather boards I had trouble getting the wake-up to _not_ trigger from electrical noise on unconnected pins. The plain ESP32 dev boards were fine, so the code should work — your mileage may vary with the hardware.

## Dependencies

- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) (1.4+)
- [HijelHID_BLEKeyboard](https://github.com/wildjames/HijelHID_BLEKeyboard/tree/multiple-clients) (the `multiple-clients` branch)
