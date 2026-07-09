#ifndef NOTEPADOS_PORTS_H
#define NOTEPADOS_PORTS_H

#include "../types.h"

/* Read a byte from an x86 I/O port. */
static inline u8 inb(u16 port) {
    u8 value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/* Write a byte to an x86 I/O port. */
static inline void outb(u16 port, u8 value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

#endif
