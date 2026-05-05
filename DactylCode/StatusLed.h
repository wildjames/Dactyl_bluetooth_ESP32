#pragma once

#include <Arduino.h>

#include "BoardConfig.h"
#include "RuntimeState.h"

namespace StatusLed {

void begin(const BoardConfig& config, LedState& ledState);
void update(const BoardConfig& config, LedState& ledState);

}
