#include "util.h"

u32 str_len(const char *s) {
    u32 n = 0;
    while (s[n] != '\0')
        n++;
    return n;
}

int str_eq(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *a == *b;
}

int str_eq_n(const char *a, const char *b, u32 n) {
    for (u32 i = 0; i < n; i++) {
        if (a[i] != b[i])
            return 0;
        if (a[i] == '\0')
            return 1;
    }
    return 1;
}

void mem_set(void *dst, u8 value, u32 n) {
    u8 *p = (u8 *)dst;
    for (u32 i = 0; i < n; i++)
        p[i] = value;
}

void str_copy(char *dst, const char *src, u32 cap) {
    u32 i = 0;
    for (; src[i] != '\0' && i + 1 < cap; i++)
        dst[i] = src[i];
    dst[i] = '\0';
}

char *int_to_str(int value, char *dst) {
    char tmp[12];
    int neg = 0;
    u32 i = 0, j = 0;

    if (value == 0) {
        dst[0] = '0';
        dst[1] = '\0';
        return dst;
    }
    if (value < 0) {
        neg = 1;
        value = -value;
    }
    while (value > 0) {
        tmp[i++] = (char)('0' + (value % 10));
        value /= 10;
    }
    if (neg)
        dst[j++] = '-';
    while (i > 0)
        dst[j++] = tmp[--i];
    dst[j] = '\0';
    return dst;
}
