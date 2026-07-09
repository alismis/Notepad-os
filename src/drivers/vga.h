#ifndef NOTEPADOS_VGA_H
#define NOTEPADOS_VGA_H

#include "../types.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* Standard VGA text-mode colors. */
enum vga_color {
    VGA_BLACK = 0, VGA_BLUE, VGA_GREEN, VGA_CYAN,
    VGA_RED, VGA_MAGENTA, VGA_BROWN, VGA_LIGHT_GREY,
    VGA_DARK_GREY, VGA_LIGHT_BLUE, VGA_LIGHT_GREEN, VGA_LIGHT_CYAN,
    VGA_LIGHT_RED, VGA_LIGHT_MAGENTA, VGA_YELLOW, VGA_WHITE
};

/* Combine a foreground and background color into an attribute byte. */
static inline u8 vga_attr(enum vga_color fg, enum vga_color bg) {
    return (u8)(fg | (bg << 4));
}

void vga_fill(u8 attr);
void vga_put(int x, int y, char c, u8 attr);
void vga_write(int x, int y, const char *s, u8 attr);
void vga_fill_rect(int x, int y, int w, int h, char c, u8 attr);
void vga_box(int x, int y, int w, int h, u8 attr);
void vga_set_cursor(int x, int y);
void vga_hide_cursor(void);

#endif
