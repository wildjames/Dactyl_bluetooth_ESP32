#pragma once

#include <Arduino.h>
#include <HijelHID_BLEKeyboard.h>

constexpr int MATRIX_KEY_COUNT = 35;

#ifndef NKEYS
#define NKEYS MATRIX_KEY_COUNT
#endif

struct LedConfig {
  int pin;
  int frequency;
  int resolution;
  int maxDutyCycle;
};

struct BatteryConfig {
  int pin;
  float refVoltage;
  float minVoltage;
  float maxVoltage;
};

struct TimingConfig {
  int pollTimeMs;
  int doubleTapIntervalMs;
  int disconnectedWaitMs;
  int disconnectedDeepSleepMs;
  int keyDelayUs;
  int deepSleepWaitMs;
  int batteryPollIntervalMs;
  int keepAliveDelayMs;
  int keepAliveLifespanMs;
};

struct BoardConfig {
  const char* boardLabel;
  const char* bleDeviceName;
  const char* primaryBleName;
  const char* manufacturerName;
  bool debug;
  bool dummy;
  bool isPrimary;
  int serialRxPin;
  int serialTxPin;
  const int* colPins;
  int colCount;
  const int* rowPins;
  int rowCount;
  const int* wakePins;
  int wakeCount;
  LedConfig led;
  BatteryConfig battery;
  TimingConfig timings;
};

extern const BoardConfig boardConfig;
extern HijelHID_BLEKeyboard bleKB;
