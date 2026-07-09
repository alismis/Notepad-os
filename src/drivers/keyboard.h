#ifndef NOTEPADOS_KEYBOARD_H
#define NOTEPADOS_KEYBOARD_H

#include "../types.h"

/* Special key codes returned in addition to printable ASCII. */
#define KEY_NONE      0
#define KEY_ENTER     '\n'
#define KEY_BACKSPACE 8
#define KEY_ESC       27
#define KEY_TAB       '\t'

/* Blocks until a key is pressed, returning printable ASCII or one of the
   special codes above. Shift is handled for letters, digits and symbols. */
char keyboard_getchar(void);

#endif
