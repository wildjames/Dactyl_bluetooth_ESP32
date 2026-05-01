#pragma once

#include <Arduino.h>

#include "BoardConfig.h"
#include "RuntimeState.h"

namespace PowerManager {

void begin(const BoardConfig& config, BatteryState& batteryState);
void update_battery_level(const BoardConfig& config, const LinkState& linkState, BatteryState& batteryState);
void enter_deep_sleep(const BoardConfig& config, LedState& ledState);

}
