/* PS/2 keyboard driver (polling). Reads scancode set 1 from port 0x60
   and translates it to ASCII, tracking the shift keys. No interrupts are
   used so the kernel stays simple; the main loop blocks on input. */
#include "keyboard.h"
#include "ports.h"

#define KBD_DATA   0x60
#define KBD_STATUS 0x64

#define SC_LSHIFT      0x2A
#define SC_RSHIFT      0x36
#define SC_LSHIFT_UP   0xAA
#define SC_RSHIFT_UP   0xB6

/* Scancode set 1 -> ASCII, unshifted. 0 means "no printable character". */
static const char keymap[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,   'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,  '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
};

/* Scancode set 1 -> ASCII, shifted. */
static const char keymap_shift[128] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,   'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
};

static int shift_down = 0;

char keyboard_getchar(void) {
    for (;;) {
        /* Wait until the output buffer holds data. */
        if (!(inb(KBD_STATUS) & 0x01))
            continue;

        u8 sc = inb(KBD_DATA);

        if (sc == SC_LSHIFT || sc == SC_RSHIFT) {
            shift_down = 1;
            continue;
        }
        if (sc == SC_LSHIFT_UP || sc == SC_RSHIFT_UP) {
            shift_down = 0;
            continue;
        }
        if (sc & 0x80)          /* key release: ignore */
            continue;
        if (sc >= 128)
            continue;

        char c = shift_down ? keymap_shift[sc] : keymap[sc];
        if (c != 0)
            return c;
    }
}
