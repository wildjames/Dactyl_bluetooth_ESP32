// To address media keys in the keymap, use negative indices that are converted to positive in the resolver.
const uint16_t media_keys[] = {
  0, 0,  // 0,1: placeholders (indices 0 and 1 are reserved)
  // Media keys
  MEDIA_PLAY,             // 2
  MEDIA_PAUSE,
  MEDIA_RECORD,
  MEDIA_FAST_FORWARD,
  MEDIA_REWIND,
  MEDIA_NEXT_TRACK,
  MEDIA_PREV_TRACK,
  MEDIA_STOP,
  MEDIA_EJECT,            // 10
  MEDIA_RANDOM_PLAY,
  MEDIA_PLAY_PAUSE,
  MEDIA_PLAY_SKIP,
  // ─── Volume Controls ───────────────────────────────────────────────────────
  MEDIA_MUTE,             // 14
  MEDIA_BASS_BOOST,
  MEDIA_VOLUME_UP,
  MEDIA_VOLUME_DOWN,
  MEDIA_BASS_UP,
  MEDIA_BASS_DOWN,
  MEDIA_TREBLE_UP,        // 20
  MEDIA_TREBLE_DOWN,

  // ─── Display / Brightness ──────────────────────────────────────────────────
  MEDIA_BRIGHTNESS_UP,    // 22
  MEDIA_BRIGHTNESS_DOWN,
  MEDIA_DISPLAY_BACKLIGHT,

  // ─── Keyboard Backlight ────────────────────────────────────────────────────
  MEDIA_KBD_BACKLIGHT_DOWN, // 25
  MEDIA_KBD_BACKLIGHT_UP,
  MEDIA_KBD_BACKLIGHT_TOGGLE,

  // ─── Application Launch ────────────────────────────────────────────────────
  MEDIA_MEDIA_SELECT,     // 28
  MEDIA_MAIL,
  MEDIA_CALCULATOR,       // 30
  MEDIA_FILE_EXPLORER,
  MEDIA_SCREENSAVER,
  MEDIA_TASK_MANAGER,

  // ─── Browser / Web Navigation ─────────────────────────────────────────────
  MEDIA_BROWSER_SEARCH,   // 34
  MEDIA_BROWSER_HOME,
  MEDIA_BROWSER_BACK,
  MEDIA_BROWSER_FORWARD,
  MEDIA_BROWSER_STOP,
  MEDIA_BROWSER_REFRESH,
  MEDIA_BROWSER_BOOKMARKS, // 40
  // ─── Power / System ────────────────────────────────────────────────────────
  MEDIA_SLEEP,
  // NOTE: MEDIA_LOCK_SCREEN uses the same HID usage as MEDIA_SCREENSAVER (0x019E,
  // AL Screen Saver). The Consumer Page has no dedicated "lock screen" usage.
  // Whether this triggers a screen lock or screensaver depends on the host OS.
  MEDIA_LOCK_SCREEN,
};
