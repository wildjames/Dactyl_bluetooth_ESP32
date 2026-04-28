uint8_t letters[] = {
  KEY_A, KEY_B, KEY_C, KEY_D, KEY_E,  // 4
  KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,  // 9
  KEY_K, KEY_L, KEY_M, KEY_N, KEY_O,  // 14
  KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,  // 19
  KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y,  // 24
  KEY_Z,                               // 25

  KEY_1, KEY_2, KEY_3, KEY_4, KEY_5,  // 30
  KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,  // 35

  KEY_MINUS, KEY_EQUAL, KEY_LEFTBRACE, KEY_RIGHTBRACE, KEY_SEMICOLON,  // 40
  KEY_APOSTROPHE, KEY_COMMA, KEY_DOT, KEY_SLASH, KEY_BACKSLASH,        // 45
  KEY_GRAVE, KEY_SPACE,                                                 // 47

  // Modifiers / navigation
  KEY_LCTRL,  KEY_LSHIFT,    KEY_LALT,      KEY_LGUI,     KEY_RCTRL,     // 52
  KEY_RSHIFT, KEY_RALT,      KEY_RGUI,      KEY_UP,       KEY_DOWN,      // 57
  KEY_LEFT,   KEY_RIGHT,     KEY_BACKSPACE, KEY_TAB,      KEY_RETURN,    // 62
  KEY_ESCAPE, KEY_INSERT,    KEY_DELETE,    KEY_PAGE_UP,  KEY_PAGE_DOWN, // 67
  KEY_HOME,   KEY_END,       KEY_CAPS_LOCK,                              // 70

  KEY_F1,  KEY_F2,  KEY_F3,  KEY_F4,  KEY_F5,  KEY_F6,   // 76
  KEY_F7,  KEY_F8,  KEY_F9,  KEY_F10, KEY_F11, KEY_F12,  // 82
  KEY_F13, KEY_F14, KEY_F15, KEY_F16, KEY_F17, KEY_F18,  // 88
  KEY_F19, KEY_F20, KEY_F21, KEY_F22, KEY_F23, KEY_F24,  // 94

  // Special chars (base keycode; KEY_MOD_LSHIFT is in letter_mods[])
  KEY_1, KEY_2, KEY_3, KEY_4, KEY_5,   // 99:  !, @, #, $, %
  KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,   // 104: ^, &, *, (, )
  KEY_MINUS, KEY_EQUAL, KEY_GRAVE,     // 107: _, +, ~
};

// Modifier byte for each letters[] entry. Non-zero only for the shifted
// special chars at indices 95–107. Consumed by bleKB.press(keycode, mod).
uint8_t letter_mods[] = {
  // 0–94: no modifier
  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,  // a–z
  0,0,0,0,0, 0,0,0,0,0,                                         // 1–0
  0,0,0,0,0, 0,0,0,0,0, 0,0,                                    // punctuation
  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,          // modifiers/nav
  0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0,          // F1–F24
  // 95–107: Shift required
  KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT,
  KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT,
  KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT,
};

const uint16_t media_keys[] = {
  MEDIA_NEXT_TRACK,    MEDIA_PREV_TRACK,  // 0,1: placeholders (indices 0 and 1 are reserved)
  // Media keys
  MEDIA_NEXT_TRACK,    MEDIA_PREV_TRACK,    MEDIA_STOP,            MEDIA_PLAY_PAUSE,   MEDIA_MUTE,
  MEDIA_VOLUME_UP,     MEDIA_VOLUME_DOWN,   MEDIA_BROWSER_HOME,    MEDIA_FILE_EXPLORER, MEDIA_CALCULATOR,
  MEDIA_BROWSER_BOOKMARKS, MEDIA_BROWSER_SEARCH, MEDIA_BROWSER_STOP, MEDIA_BROWSER_BACK, MEDIA_MEDIA_SELECT,
  MEDIA_MAIL,
};
