char letters[] = {
  'a', 'b', 'c', 'd', 'e',  
  'f', 'g', 'h', 'i', 'j',  
  'k', 'l', 'm', 'n', 'o',  
  'p', 'q', 'r', 's', 't',  
  'u', 'v', 'w', 'x', 'y',  
  'z',

  '1', '2', '3', '4', '5',  
  '6', '7', '8', '9', '0',  

  '-', '=', '[', ']', ';',
  '\'', ',', '.', '/', '\\',
  '`', ' ',
  
  // Modifiers
  KEY_LEFT_CTRL,   KEY_LEFT_SHIFT,  KEY_LEFT_ALT,   KEY_LEFT_GUI,   KEY_RIGHT_CTRL, 
  KEY_RIGHT_SHIFT, KEY_RIGHT_ALT,   KEY_RIGHT_GUI,  KEY_UP_ARROW,   KEY_DOWN_ARROW, 
  KEY_LEFT_ARROW,  KEY_RIGHT_ARROW, KEY_BACKSPACE,  KEY_TAB,        KEY_RETURN,     
  KEY_ESC,         KEY_INSERT,      KEY_DELETE,     KEY_PAGE_UP,    KEY_PAGE_DOWN,  
  KEY_HOME,        KEY_END,         KEY_CAPS_LOCK,                                      
  
  KEY_F1,        KEY_F2,         KEY_F3,       KEY_F4,         KEY_F5,         KEY_F6,  
  KEY_F7,        KEY_F8,         KEY_F9,       KEY_F10,        KEY_F11,        KEY_F12, 
  KEY_F13,       KEY_F14,        KEY_F15,      KEY_F16,        KEY_F17,        KEY_F18,
  KEY_F19,       KEY_F20,        KEY_F21,      KEY_F22,        KEY_F23,        KEY_F24,

  // Special chars
  '!', '@', '#', '$', '%',
  '^', '&', '*', '(', ')',
  '_', '+', '~',
};

const uint8_t* media_keys[] = {
  KEY_MEDIA_NEXT_TRACK,    KEY_MEDIA_PREVIOUS_TRACK, // These two are ignored, zero and 1 are flags for other things so we have to fill them with junk here
  // Media keys and such
  KEY_MEDIA_NEXT_TRACK,    KEY_MEDIA_PREVIOUS_TRACK, KEY_MEDIA_STOP,     KEY_MEDIA_PLAY_PAUSE,            KEY_MEDIA_MUTE,       
  KEY_MEDIA_VOLUME_UP,     KEY_MEDIA_VOLUME_DOWN,    KEY_MEDIA_WWW_HOME, KEY_MEDIA_LOCAL_MACHINE_BROWSER, KEY_MEDIA_CALCULATOR, 
  KEY_MEDIA_WWW_BOOKMARKS, KEY_MEDIA_WWW_SEARCH,     KEY_MEDIA_WWW_STOP, KEY_MEDIA_WWW_BACK,              KEY_MEDIA_CONSUMER_CONTROL_CONFIGURATION, 
  KEY_MEDIA_EMAIL_READER, 
};



// Layers can be accessed by holding the MOD key, index defined below
int MODKEY0 = 32;

// Can toggle to a gaming-focussed layout with these keys. Made them different keys so that
// you *know* that no matter what state you were in before, now you're in the new one.
// If this is negative, toggling is disabled.
int alt_toggle = -1;
int typing_toggle = -1;

// Keys per half, NCOLS * NROWS, must be calculated by the user.
#define NKEYS 35

int keymap[] = {
  // Needs to be the index of the desired key in the letters array. 
  // Layers are accessed by adding NKEYS to the pressed switch ID, 
  // so this array needs to be NKEYS * NLAYERS long.
  
  // Layer 0
   60,   35,   34,   33,   31, 
   32,   46,   36,   15,   14, 
    9,    7,   20,   69,   41, 
   40,   11,   10,   -1,   -1, 
   47,   45,   44,   43,   42, 
   13,   12,   61,   -1,   -1, 
   -1,   -1,   -1,   -1,   62, 
 
  // Layer 1
   65,   80,   79,   78,   76, 
   77,   -1,   81,   36,   34, 
   29,   -1,   32,   -1,   82, 
  106,   31,   30,   -1,   -1, 
   -1,   -1,   62,   28,   27, 
   -1,   26,   -1,   -1,   -1, 
   -1,   -1,   -1,   -1,   -1, 
};

int alt_keymap[] = {

};
