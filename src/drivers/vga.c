/* VGA text-mode driver: writes directly to the framebuffer at 0xB8000
   and drives the hardware cursor through CRT controller ports. */
#include "vga.h"
#include "ports.h"

static volatile u16 *const VGA_MEM = (volatile u16 *)0xB8000;

static inline u16 cell(char c, u8 attr) {
    return (u16)((u8)c) | ((u16)attr << 8);
}

void vga_fill(u8 attr) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_MEM[i] = cell(' ', attr);
}

void vga_put(int x, int y, char c, u8 attr) {
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT)
        return;
    VGA_MEM[y * VGA_WIDTH + x] = cell(c, attr);
}

void vga_write(int x, int y, const char *s, u8 attr) {
    for (int i = 0; s[i] != '\0'; i++)
        vga_put(x + i, y, s[i], attr);
}

void vga_fill_rect(int x, int y, int w, int h, char c, u8 attr) {
    for (int row = 0; row < h; row++)
        for (int col = 0; col < w; col++)
            vga_put(x + col, y + row, c, attr);
}

/* Draw a single-line box border using CP437 box-drawing characters. */
void vga_box(int x, int y, int w, int h, u8 attr) {
    const char TL = (char)0xC9, TR = (char)0xBB, BL = (char)0xC8, BR = (char)0xBC;
    const char HZ = (char)0xCD, VT = (char)0xBA;

    vga_put(x, y, TL, attr);
    vga_put(x + w - 1, y, TR, attr);
    vga_put(x, y + h - 1, BL, attr);
    vga_put(x + w - 1, y + h - 1, BR, attr);
    for (int col = 1; col < w - 1; col++) {
        vga_put(x + col, y, HZ, attr);
        vga_put(x + col, y + h - 1, HZ, attr);
    }
    for (int row = 1; row < h - 1; row++) {
        vga_put(x, y + row, VT, attr);
        vga_put(x + w - 1, y + row, VT, attr);
    }
}

void vga_set_cursor(int x, int y) {
    u16 pos = (u16)(y * VGA_WIDTH + x);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8)((pos >> 8) & 0xFF));
}

void vga_hide_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20); /* bit 5 disables the cursor */
}
