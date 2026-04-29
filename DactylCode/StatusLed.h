#pragma once

#include <Arduino.h>

#include "BoardConfig.h"
#include "KeymapResolver.h"
#include "RuntimeState.h"

namespace StatusLed {

void begin(const BoardConfig& config, LedState& ledState);
void show_connected(const BoardConfig& config, LedState& ledState, const KeymapResolver::KeyboardState& keyboardState);
void show_disconnected(const BoardConfig& config, LedState& ledState);
void turn_off(const BoardConfig& config, LedState& ledState);

}
