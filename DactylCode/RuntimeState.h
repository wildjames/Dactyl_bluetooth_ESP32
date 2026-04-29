#pragma once

#include <Arduino.h>
#include <stdint.h>

struct MatrixState {
  int keyStates[NKEYS] = { 0 };
  int previousKeyStates[NKEYS] = { 0 };
};

struct LinkState {
  bool splitCommunication = false;
  bool useGatt = false;
  bool isConnected = false;
  int keepAliveMessage = 50;
  unsigned long lastKeepAliveCheck = 0;
  unsigned long lastKeepAliveTime = 0;
  uint8_t lastGattConnectedCount = 0;
};

struct BatteryState {
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
