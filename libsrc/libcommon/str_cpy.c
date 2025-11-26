#include <stdint.h>

void* str_cpy(void* dst, const void* src) {
    uint8_t* d = dst;
    const uint8_t* s = src;

    while(*s != 0x00) *d++ = *s++;
    *d = 0x00;
    return dst;
}