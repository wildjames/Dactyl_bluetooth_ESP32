#pragma once

#include <Arduino.h>

#include "BoardConfig.h"
#include "RuntimeState.h"

namespace MatrixScanner {

void release_sleep_matrix_config(const BoardConfig& config);
void configure_pins(const BoardConfig& config);
void scan(const BoardConfig& config, MatrixState& matrixState);
uint64_t build_wake_pin_mask(const BoardConfig& config);
void prepare_wake_pins(const BoardConfig& config);

}
