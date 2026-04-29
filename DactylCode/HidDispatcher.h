#pragma once

#include <Arduino.h>

#include "BoardConfig.h"
#include "KeymapResolver.h"

namespace HidDispatcher {

void begin();
bool has_host_connection();
void set_battery_level(float batteryPercentage);
void release_all();
void tap_caps_lock();
void tap_media(uint16_t mediaCode, bool dummy);
void release_media(uint16_t mediaCode, bool dummy);
void press_key(uint8_t keycode, uint8_t modifier, bool dummy);
void release_key(uint8_t keycode, uint8_t modifier, bool dummy);
void press_passthrough(uint8_t keycode, bool dummy);
void release_passthrough(uint8_t keycode);
void dispatch_action(const KeymapResolver::Action& action, bool dummy);

}
