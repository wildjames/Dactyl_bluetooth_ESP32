# Dactyl Manuform with bluetooth via. ESP32

The dactly manuform keyboard - now with easy bluetooth

This is by no means a complete, easy-to-replicate project. QMK currently does not support the bluetooth-capable ESP32 microcontroller, due to an allegedly "easier" bluetooth implimentation using a pro micro and a bluetooth breakout board. Doesn't sound easier to me, though. The Arduino code in this repo is just a stop-gap until that gets implimented on the *far* more robust QMK firmware.

The [https://github.com/abstracthat/dactyl-manuform#:~:text=README.md-,The%20Dactyl%2DManuForm%20Keyboard,just%20drop%20to%20the%20floor.](Dactyl Manuform) is a great split keyboard, with a pretty non-standard form. Most pertinently, it's a split keyboard, typically with the two halves having their own microcontrollers that communicate over a TRRS connection, or something similar. 
I wanted a wireless solution, and preferably one that's a bit more flexible. This is the result, and is thanks to the excellent work in getting the ESP32 HID mode running by [https://github.com/T-vK/ESP32-BLE-Keyboard](these guys). 

If you've seen the dactyl before and you're on the fence, I'd say pull the trigger. Learning the layout is worth the trouble, and with some nice clicky blues this is a great keyboard.

## Broad board overview 

Each half uses an ESP32 to handle all the thinking, and both halves independantly send their keystrokes to a connected device over bluetooth. 
Notably, I've built in the option for inter-keyboard communication and sending keystrokes only from a master half. This would let you use a modifier on one half to access a layer on the opposing half, which some people like. 
For my usecase, this isn't necessary and I've repurposed the communication wire to share a charging current between the two keyboards - i.e. if one half is plugged in and charging, and the other half is connected to the first, it will also charge. 
There's also a single alternate layer modifier key built in for each half, but I left it at one additional layer because I can never remember any more than that. Finally, there's a toggle-able layer that can be accessed from a chosen key, for alternate layouts, e.g. a QWERTY/Dvorak toggle key.

I initially used plain old ESP32 dev boards, but the cheap breakout battery circuit I had drew far too much current even when the controller was asleep. I caved and used a couple of Feather HUZZAH32 instead. If you're making this project, I'd recommend the same! It's not best deep sleep current draw, but off my 4000mAh batteries I get standby times in the ~month range. Plus, Adafruit sacrifice an analog pin for battery monitoring with no extra work on your behalf, so updating your battery level is trivial.

I used some RJ9 connectors to join the halves, and USB-C to supply current/USB to the controllers, though note that keystrokes are **never** sent over USB!

## Tayloring the code for yourself

Each half has a BoardConfig.h file, that contains all the user-ediable stuff you should need. I hope most of it is self-explanatory, but there are a couple of Gotchas.

### Making keymaps for my code

I designed this for a 5x6 manuform, requiring 5 + 7 GPIO (5x6 for the main keys, and one more for the thumb cluster). The code should work fine with other layouts, though, just remember to change `NKEYS` in the config!. If you need help wiring a keyboard, there are plenty of guides - I'd not recommend something like this for a first keyboard!

The way I've written the code, making keyboard layouts is not fun. The way keyboards generally work is to poll each key for if it's pressed, then parse out what keystroke to send. I do this by finding all the indexes of pressed keys (e.g. the key attached to row 2 and column 5 would be index 

I've written a python helper script to convert QMK's slightly less insane JSON keymap format into my required form, but when I wired the key rows and columns, I wasn't smart about it and you'll need to read the python code comments if you want to use that. Sorry!

A less painful, but slow way to make a keymap is to enable the DEBUG and DUMMY modes in the arduino code. Then, hitting a key will tell you its index and you can fill the keymap array that way. Alternatively, there's a keymapper sketch that *should* prompt you for keys, and build a keymap for you that way. It'll only work for the keys it asks for, though, so no exotic stuff like play/pause, or whatever. 

### The LED pin

I didn't want illumination on all keys because of the power consumption factor, but an indication LED is still nice. I wired one underneath the escape and backspace keys for each half, and these will flash when the keyboard is disconnected or be solidly illuminated when paired. If the LED is off, your battery is dead (todo: impliment a more elegant solution...) This is in the config as "LEDPin". 

### Waking from deepsleep

After some amount of idle time, the keyboard halves will enter deepsleep to save power. However, in the interests of convinence, I wanted to be able to mash keys to wake them back up. The RTC pins can be held high during deepsleep, though I suspect at a cost to battery life, but for my feather boards I couldn't get the wakup to *not* trigger even on things like touching an unconnected wire to the wakeup pins... The dev boards worked okay though, so the code should be fine. 

