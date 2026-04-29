#include "KeymapResolver.h"

namespace KeymapResolver {

namespace {

void push_action(Result& result, ActionType type, uint8_t keycode = 0, uint8_t modifier = 0, uint16_t mediaCode = 0) {
  if (result.actionCount >= (int)(sizeof(result.actions) / sizeof(result.actions[0]))) {
    return;
  }

  Action& action = result.actions[result.actionCount++];
  action.type = type;
  action.keycode = keycode;
  action.modifier = modifier;
  action.mediaCode = mediaCode;
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
    if ((millis() - keyboardState.lastModTap) < (unsigned long)config.doubleTapIntervalMs) {
      keyboardState.lockedModKey = !keyboardState.lockedModKey;
      keyboardState.lastModTap -= config.doubleTapIntervalMs;
    } else {
      keyboardState.lastModTap = millis();
    }
  }

  if (config.shiftKeyIndex >= 0
      && matrixState.previousKeyStates[config.shiftKeyIndex]
      && !matrixState.keyStates[config.shiftKeyIndex]) {
    if ((millis() - keyboardState.lastShiftTap) < (unsigned long)config.doubleTapIntervalMs) {
      push_action(result, ActionType::TapCapsLock);
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
      push_action(result, ActionType::ReleaseAll);
      return true;
    }
    return false;
  }

  if (config.altToggleKeyIndex >= 0 && matrixState.keyStates[config.altToggleKeyIndex]) {
    keyboardState.isAltLayout = true;
    push_action(result, ActionType::ReleaseAll);
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

  int alternateLetterIndex = keymap[alternateIndex];
  if (alternateLetterIndex == -1) {
    return;
  }

  if (alternateLetterIndex < -1) {
    alternateLetterIndex *= -1;
    push_action(result, ActionType::MediaRelease, 0, 0, config.mediaKeys[alternateLetterIndex]);
    return;
  }

  push_action(
    result,
    ActionType::KeyRelease,
    config.keycodes[alternateLetterIndex],
    config.keyModifiers[alternateLetterIndex]
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

      int letterIndex = activeKeymap[pressedIndex];
      if (letterIndex == -1) {
        continue;
      }

      if (letterIndex < -1) {
        letterIndex *= -1;
        push_action(result, ActionType::MediaTap, 0, 0, config.mediaKeys[letterIndex]);
        continue;
      }

      keyboardState.lastKeypress = millis();
      push_action(
        result,
        ActionType::KeyPress,
        config.keycodes[letterIndex],
        config.keyModifiers[letterIndex]
      );
    }

    if (!matrixState.keyStates[i] && matrixState.previousKeyStates[i]) {
      int pressedIndex = pressedOffset + i;
      if (!within_keymap_bounds(pressedIndex, keymapLength)) {
        continue;
      }

      int letterIndex = activeKeymap[pressedIndex];
      if (letterIndex == -1) {
        continue;
      }

      if (letterIndex < -1) {
        letterIndex *= -1;
        push_action(result, ActionType::MediaRelease, 0, 0, config.mediaKeys[letterIndex]);
        continue;
      }

      push_action(
        result,
        ActionType::KeyRelease,
        config.keycodes[letterIndex],
        config.keyModifiers[letterIndex]
      );
      if (config.modifierKeyIndex >= 0) {
        append_release_for_alternate_layer(pressedIndex, matrixState, keyboardState, config, activeKeymap, keymapLength, result);
      }
    }
  }
}

}
