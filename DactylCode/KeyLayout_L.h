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
   63,   61,   49,   48,   38, 
   26,   16,    0,   25,   39, 
   27,   22,   18,   23,   51, 
   28,    4,    3,    2,   50, 
   29,   17,    5,   21,   -1, 
   30,   19,    6,    1,   -1, 
   68,   62,   -1,   47,   -1, 
 
  // Layer 1
  107,   -1,   70,   -1,   -1, 
   71,   -1,   -1,   -1,   -1, 
   72,   66,   58,   -1,   -1, 
   73,   56,   57,   -1,   -1, 
   74,   67,   59,   -1,   -1, 
   75,   -1,   -1,   -1,   -1, 
   -1,   -1,   -1,   -1,   -1, 
};

int alt_keymap[] = {

};
