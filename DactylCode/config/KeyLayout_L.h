// Layers can be accessed by holding the MOD key, index defined below
int MODKEY0 = 32;

// Double-tap this physical key index to toggle caps lock. Set to -1 to disable.
int SHIFTKEY0 = 2;

// Can toggle to a gaming-focussed layout with these keys. Made them different keys so that
// you *know* that no matter what state you were in before, now you're in the new one.
// If this is negative, toggling is disabled.
int alt_toggle = -1;
int typing_toggle = -1;

int keymap[] = {
  // Keymap values are HID keycodes directly.
  // -1 = no key assigned, negative < -1 = media key (negate to get media_keys[] index).
  // Layers are accessed by adding NKEYS to the pressed switch ID,
  // so this array needs to be NKEYS * NLAYERS long.

  // Layer 0
  KEY_ESCAPE,    KEY_TAB,    KEY_LSHIFT,  KEY_LCTRL,    KEY_LEFTBRACE,
  KEY_1,         KEY_Q,      KEY_A,       KEY_Z,        KEY_RIGHTBRACE,
  KEY_2,         KEY_W,      KEY_S,       KEY_X,        KEY_LGUI,
  KEY_3,         KEY_E,      KEY_D,       KEY_C,        KEY_LALT,
  KEY_4,         KEY_R,      KEY_F,       KEY_V,        -1,
  KEY_5,         KEY_T,      KEY_G,       KEY_B,        -1,
  KEY_HOME,      KEY_RETURN, -1,          KEY_SPACE,    -1,

  // Layer 1
  KEY_ESCAPE,    KEY_TAB,      KEY_LSHIFT,    KEY_LCTRL,     -1,
  KEY_F1,        -1,           -1,            -1,            -1,
  KEY_F2,        KEY_PAGE_UP,  KEY_LEFT,      -1,            KEY_LGUI,
  KEY_F3,        KEY_UP,       KEY_DOWN,      -1,            KEY_LALT,
  KEY_F4,        KEY_PAGE_DOWN,KEY_RIGHT,     -1,            -1,
  KEY_F5,        -1,           -1,            -1,            -1,
  KEY_HOME,      KEY_RETURN,   -1,            KEY_SPACE,     -1,
};

int alt_keymap[] = {};
