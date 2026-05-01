#include "MatrixScanner.h"

#include "driver/gpio.h"
#include "driver/rtc_io.h"

namespace MatrixScanner {

void release_sleep_matrix_config(const BoardConfig& config) {
  gpio_deep_sleep_hold_dis();

  for (int i = 0; i < config.rowCount; i++) {
    gpio_num_t row_pin = (gpio_num_t)config.rowPins[i];
    gpio_hold_dis(row_pin);
    if (rtc_gpio_is_valid_gpio(row_pin)) {
      rtc_gpio_deinit(row_pin);
    }
  }

  for (int i = 0; i < config.wakeCount; i++) {
    gpio_num_t wake_pin = (gpio_num_t)config.wakePins[i];
    gpio_hold_dis(wake_pin);
    if (rtc_gpio_is_valid_gpio(wake_pin)) {
      rtc_gpio_deinit(wake_pin);
    }
  }
}

void configure_pins(const BoardConfig& config) {
  if (config.debug) { Serial.println("Row nums:"); }
  for (int i = 0; i < config.rowCount; i++) {
    int row_pin = config.rowPins[i];
    if (config.debug) { Serial.println(row_pin); }
    pinMode(row_pin, OUTPUT);
    gpio_hold_dis((gpio_num_t)row_pin);
    digitalWrite(row_pin, LOW);
  }

  if (config.debug) { Serial.println("Col nums:"); }
  for (int i = 0; i < config.colCount; i++) {
    int col_pin = config.colPins[i];
    if (config.debug) { Serial.println(col_pin); }
    pinMode(col_pin, INPUT_PULLDOWN);
  }
}

void scan(const BoardConfig& config, MatrixState& matrixState) {
  int key_index = 0;

  for (int i = 0; i < config.rowCount; i++) {
    digitalWrite(config.rowPins[i], HIGH);
    for (int j = 0; j < config.colCount; j++) {
      bool value = digitalRead(config.colPins[j]);
      matrixState.previousKeyStates[key_index] = matrixState.keyStates[key_index];
      matrixState.keyStates[key_index] = value;
      key_index++;
    }
    digitalWrite(config.rowPins[i], LOW);
    delayMicroseconds(config.timings.keyDelayUs);
  }
}

uint64_t build_wake_pin_mask(const BoardConfig& config) {
  uint64_t wake_pin_mask = 0;
  for (int i = 0; i < config.wakeCount; i++) {
    wake_pin_mask |= (1ULL << config.wakePins[i]);
  }
  return wake_pin_mask;
}

void prepare_wake_pins(const BoardConfig& config) {
  for (int i = 0; i < config.rowCount; i++) {
    digitalWrite(config.rowPins[i], HIGH);
    gpio_hold_en((gpio_num_t)config.rowPins[i]);
  }
  gpio_deep_sleep_hold_en();

  for (int i = 0; i < config.wakeCount; i++) {
    gpio_num_t wake_pin = (gpio_num_t)config.wakePins[i];
    rtc_gpio_init(wake_pin);
    rtc_gpio_set_direction(wake_pin, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_dis(wake_pin);
    rtc_gpio_pulldown_en(wake_pin);
  }
}

}
