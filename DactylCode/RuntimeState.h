#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "BoardConfig.h"

struct MatrixState {
  int keyStates[MATRIX_KEY_COUNT] = { 0 };
  int previousKeyStates[MATRIX_KEY_COUNT] = { 0 };
};

struct LinkState {
  bool allowGatt = false;
  bool isConnected = false;
  uint8_t lastGattConnectedCount = 0;
};

struct BatteryState {
  bool monitorAvailable = false;
  float voltage = NAN;
  float percentage = NAN;
  float chargeRate = NAN;
  float companionPercentage = NAN;
  unsigned long lastUpdate = 0;
};

struct ModifierState {
  unsigned long lastModTap = 0;
  bool lockedModKey = false;
  unsigned long lastModFlash = 0;
  bool modFlashHigh = false;
  unsigned long lastShiftTap = 0;
  bool isAltLayout = false;
};

struct LoopState {
  unsigned long lastLoop = 0;
  unsigned long lastKeypress = 0;
};

struct LedState {
  int outputState = HIGH;
  int dutyCycle = 0;
  unsigned long lastFlashToggle = 0;
  bool flashHigh = false;
};

struct RuntimeState {
  MatrixState matrix;
  LinkState link;
  BatteryState battery;
  ModifierState modifiers;
  LoopState loop;
  LedState led;
};

extern RuntimeState runtimeState;
extern LinkState& linkState;
