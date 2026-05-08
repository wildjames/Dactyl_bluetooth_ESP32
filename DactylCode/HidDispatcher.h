#pragma once

#include <Arduino.h>

#include "BoardConfig.h"
#include "KeymapResolver.h"
#include "RuntimeState.h"

namespace HidDispatcher {

void begin();
void begin_usb();
bool has_host_connection();
bool has_usb_connection();
void set_battery_level(float batteryPercentage);
void release_all(ConnectionType conn);
void tap_key(uint8_t keyCode, bool dummy, ConnectionType conn);
void press_key(uint8_t keycode, bool dummy, ConnectionType conn);
void release_key(uint8_t keycode, bool dummy, ConnectionType conn);
void dispatch_action(const KeymapResolver::Action& action, bool dummy, ConnectionType conn);
void before_sleep();

}
