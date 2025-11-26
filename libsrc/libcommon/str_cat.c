#include <stdint.h>

void* str_cat(void* dst, const void* src) {
    uint8_t* d = dst;
    const uint8_t* s = src;

    while(*d != 0x00) *d++;
    while(*s != 0x00) *d++ = *s++;
    *d = 0x00;
    return dst;
}