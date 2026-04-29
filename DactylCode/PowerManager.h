#pragma once

#include <Arduino.h>

#include "BoardConfig.h"
#include "RuntimeState.h"

namespace PowerManager {

void update_battery_level(const BoardConfig& config, const LinkState& linkState, BatteryState& batteryState);
void enter_deep_sleep(const BoardConfig& config, LedState& ledState);

}
