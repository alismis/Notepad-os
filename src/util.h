#ifndef NOTEPADOS_UTIL_H
#define NOTEPADOS_UTIL_H

#include "types.h"

u32  str_len(const char *s);
int  str_eq(const char *a, const char *b);
int  str_eq_n(const char *a, const char *b, u32 n);
void mem_set(void *dst, u8 value, u32 n);
void str_copy(char *dst, const char *src, u32 cap);
/* Convert a signed integer to a decimal string. Returns dst. */
char *int_to_str(int value, char *dst);

#endif
