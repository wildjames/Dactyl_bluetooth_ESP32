#include "KeymapResolver.h"

namespace KeymapResolver {

namespace {

void push_action(Result& result, uint8_t keyIndex, ActionType type, uint8_t keycode = 0) {
  if (result.actionCount >= (int)(sizeof(result.actions) / sizeof(result.actions[0]))) {
    return;
  }

  Action& action = result.actions[result.actionCount++];
  action.type = type;
  action.keyIndex = keyIndex;
  action.keycode = keycode;
}

bool within_keymap_bounds(int index, int keymapLength) {
  return index >= 0 && index < keymapLength;
}

const int* select_active_keymap(const KeyboardState& keyboardState, const Config& config, int& keymapLength) {
  if (keyboardState.isAltLayout && config.alternateKeymapLength > 0) {
    keymapLength = config.alternateKeymapLength;
    return config.alternateKeymap;
  }

  keymapLength = config.primaryKeymapLength;
  return config.primaryKeymap;
}

void update_modifier_lock(MatrixState& matrixState, KeyboardState& keyboardState, const Config& config, Result& result) {
  if (config.modifierKeyIndex >= 0
      && matrixState.previousKeyStates[config.modifierKeyIndex]
      && !matrixState.keyStates[config.modifierKeyIndex]) {
    unsigned long modTapSeparationMs = millis() - keyboardState.lastModTap;
    if (modTapSeparationMs >= (unsigned long)config.doubleTapMinIntervalMs
        && modTapSeparationMs < (unsigned long)config.doubleTapIntervalMs) {
      keyboardState.lockedModKey = !keyboardState.lockedModKey;
      keyboardState.lastModTap -= config.doubleTapIntervalMs;
    } else {
      keyboardState.lastModTap = millis();
    }
  }

  if (config.shiftKeyIndex >= 0
      && matrixState.previousKeyStates[config.shiftKeyIndex]
      && !matrixState.keyStates[config.shiftKeyIndex]) {
    unsigned long shiftTapSeparationMs = millis() - keyboardState.lastShiftTap;
    if (shiftTapSeparationMs >= (unsigned long)config.doubleTapMinIntervalMs
        && shiftTapSeparationMs < (unsigned long)config.doubleTapIntervalMs) {
      push_action(result, config.shiftKeyIndex, ActionType::TapCapsLock); // TODO: This should use the tap action
      keyboardState.lastShiftTap -= config.doubleTapIntervalMs;
    } else {
      keyboardState.lastShiftTap = millis();
    }
  }
}

bool handle_layout_toggle(MatrixState& matrixState, KeyboardState& keyboardState, const Config& config, Result& result) {
  if (keyboardState.isAltLayout) {
    if (config.typingToggleKeyIndex >= 0 && matrixState.keyStates[config.typingToggleKeyIndex]) {
      keyboardState.isAltLayout = false;
      matrixState.previousKeyStates[config.typingToggleKeyIndex] = 1;
      push_action(result, config.typingToggleKeyIndex, ActionType::ReleaseAll);
      return true;
    }
    return false;
  }

  if (config.altToggleKeyIndex >= 0 && matrixState.keyStates[config.altToggleKeyIndex]) {
    keyboardState.isAltLayout = true;
    push_action(result, config.altToggleKeyIndex, ActionType::ReleaseAll);
    return true;
  }

  return false;
}

void append_release_for_alternate_layer(int pressedIndex, const MatrixState& matrixState, const KeyboardState& keyboardState, const Config& config, const int* keymap, int keymapLength, Result& result) {
  int alternateIndex = keyboardState.lockedModKey || matrixState.keyStates[config.modifierKeyIndex]
    ? pressedIndex - MATRIX_KEY_COUNT
    : pressedIndex + MATRIX_KEY_COUNT;

  if (!within_keymap_bounds(alternateIndex, keymapLength)) {
    return;
  }

  int alternateKeyCode = keymap[alternateIndex];
  if (alternateKeyCode == -1) {
    return;
  }

  // TODO: Refactor this alongside the tap handling in resolve()
  // if (alternateKeyCode < -1) {
  //   alternateKeyCode *= -1;
  //   push_action(result, alternateIndex, ActionType::KeyRelease, 0);
  //   return;
  // }

  push_action(
    result,
    alternateIndex,
    ActionType::KeyRelease,
    (uint8_t)alternateKeyCode
  );
}

}

void resolve(MatrixState& matrixState, KeyboardState& keyboardState, const Config& config, Result& result) {
  result.actionCount = 0;

  update_modifier_lock(matrixState, keyboardState, config, result);
  if (handle_layout_toggle(matrixState, keyboardState, config, result)) {
    return;
  }

  int keymapLength = 0;
  const int* activeKeymap = select_active_keymap(keyboardState, config, keymapLength);
  if (activeKeymap == nullptr || keymapLength <= 0) {
    return;
  }

  int pressedOffset = 0;
  if (config.modifierKeyIndex >= 0
      && (matrixState.keyStates[config.modifierKeyIndex] || keyboardState.lockedModKey)) {
    pressedOffset += MATRIX_KEY_COUNT;
  }

  for (int i = 0; i < MATRIX_KEY_COUNT; i++) {
    if (matrixState.keyStates[i] && !matrixState.previousKeyStates[i]) {
      int pressedIndex = pressedOffset + i;
      if (!within_keymap_bounds(pressedIndex, keymapLength)) {
        continue;
      }

      int keyCode = activeKeymap[pressedIndex];
      if (keyCode == -1) {
        continue;
      }

      // TODO
      // I need to think about how to define that a key should be tapped instead of pressed.
      // The negative index worked for a separate array of media keys, but it's not the right solution anymore.
      // if (keyCode < -1) {
      //   keyCode *= -1;
      //   push_action(result, pressedIndex, ActionType::KeyTap, 0);
      //   continue;
      // }

      keyboardState.lastKeypress = millis();
      push_action(
        result,
        pressedIndex,
        ActionType::KeyPress,
        (uint8_t)keyCode
      );
    }

    if (!matrixState.keyStates[i] && matrixState.previousKeyStates[i]) {
      int pressedIndex = pressedOffset + i;
      if (!within_keymap_bounds(pressedIndex, keymapLength)) {
        continue;
      }

      int keyCode = activeKeymap[pressedIndex];
      if (keyCode == -1) {
        continue;
      }

      push_action(
        result,
        pressedIndex,
        ActionType::KeyRelease,
        (uint8_t)keyCode
      );
      if (config.modifierKeyIndex >= 0) {
        append_release_for_alternate_layer(pressedIndex, matrixState, keyboardState, config, activeKeymap, keymapLength, result);
      }
    }
  }
}

}
