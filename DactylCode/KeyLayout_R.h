// This is NOT the actual key layout! This is just a key, to the key layout. 
#include "KeyLayout.h"


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
    8,   24,   20,   69,   41, 
   40,   11,   10,    7,    9, 
   47,   45,   44,   43,   42, 
   13,   12,   61,   -1,   -1, 
   37,  102,   -1,   -1,   62, 
 
  // Layer 1
   65,   80,   79,   78,   76, 
   77,   -1,   81,   36,   34, 
   33,   -1,   32,   -1,   82, 
  106,   31,   30,   -1,   29, 
   -1,   64,   62,   28,   27, 
   -1,   26,   -1,   -1,   -1, 
   43,   35,   -1,   -1,   -1, 
};

int alt_keymap[] = {

};
