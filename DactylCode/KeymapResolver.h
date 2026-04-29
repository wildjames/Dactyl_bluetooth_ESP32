#pragma once

#include <Arduino.h>

#include "RuntimeState.h"

namespace KeymapResolver {

enum class ActionType {
  None,
  ReleaseAll,
  TapCapsLock,
  KeyPress,
  KeyRelease,
  MediaTap,
  MediaRelease,
};

struct KeyboardState {
  unsigned long lastModTap = 0;
  unsigned long lastShiftTap = 0;
  unsigned long lastKeypress = 0;
  bool lockedModKey = false;
  bool isAltLayout = false;
};

struct Action {
  ActionType type = ActionType::None;
  uint8_t keycode = 0;
  uint8_t modifier = 0;
  uint16_t mediaCode = 0;
};

struct Config {
  int modifierKeyIndex = -1;
  int shiftKeyIndex = -1;
  int altToggleKeyIndex = -1;
  int typingToggleKeyIndex = -1;
  int doubleTapIntervalMs = 0;
  const int* primaryKeymap = nullptr;
  int primaryKeymapLength = 0;
  const int* alternateKeymap = nullptr;
  int alternateKeymapLength = 0;
  const uint8_t* keycodes = nullptr;
  const uint8_t* keyModifiers = nullptr;
  const uint16_t* mediaKeys = nullptr;
};

struct Result {
  Action actions[MATRIX_KEY_COUNT * 3 + 4];
  int actionCount = 0;
};

void resolve(MatrixState& matrixState, KeyboardState& keyboardState, const Config& config, Result& result);

}
