// Layers can be accessed by holding the MOD key, index defined below
int MODKEY0 = 28;

int SHIFTKEY0 = -1;

// Can toggle to a gaming-focussed layout with these keys. Made them different keys so that
// you *know* that no matter what state you were in before, now you're in the new one.
// If this is negative, toggling is disabled.
int alt_toggle = -1;
int typing_toggle = -1;

int keymap[] = {
  KEY_6,      KEY_7,      KEY_8,      KEY_9,       KEY_0,     KEY_BACKSPACE,
  KEY_GRAVE, // Thumb cluster keys are at the end of each row
  KEY_Y,      KEY_U,      KEY_I,      KEY_O,       KEY_P,     KEY_MINUS,
  KEY_END,
  KEY_H,      KEY_J,      KEY_K,      KEY_L,       KEY_SEMICOLON, KEY_APOSTROPHE,
  KEY_SPACE,
  KEY_N,      KEY_M,      KEY_COMMA,  KEY_DOT,     KEY_SLASH,  KEY_RSHIFT,
  KEY_BACKSLASH, -1, -1, // The key at index 28 is the modifier key and 29 is no connect
  MEDIA_PLAY_PAUSE, KEY_EQUAL,
  -1, -1, KEY_RETURN, // Key 34 is the enter key on the thumb cluster

  // Layer 1
  // Number rows become F6-F10, minus becomes F11 and apostrophe becomes F12. Right shift becomes insert and equals becomes the dot. Numbers are M=1, ,=2, .=3, j=4, etc
  KEY_F6,     KEY_F7,     KEY_F8,     KEY_F9,      KEY_F10,    KEY_DELETE,
  KEY_GRAVE,
  -1,         KEY_KP_7,   KEY_KP_8,   KEY_KP_9,    KEY_KP_PLUS,   KEY_F11,
  KEY_END,
  -1,         KEY_KP_4,   KEY_KP_5,   KEY_KP_6,    KEY_KP_MINUS,  KEY_F12,
  KEY_SPACE,
  -1,         KEY_KP_1,   KEY_KP_2,   KEY_KP_3,    -1,             KEY_INSERT,
  KEY_BACKSLASH, -1, -1,
  KEY_KP_0, KEY_KP_DOT,
  -1, -1, KEY_RETURN,
};

int alt_keymap[] = {};
