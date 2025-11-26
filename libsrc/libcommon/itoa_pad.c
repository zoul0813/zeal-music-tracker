#include <stdint.h>
#include "common.h"

void itoa_pad(uint16_t num, char* str, uint16_t base, char alpha, char pad, uint8_t size) {
    itoa(num, str, base, alpha);
    uint16_t l = str_len(str);
    if (l >= size) return;

    uint16_t pad_count = size - l;
    for (int16_t i = l; i >= 0; i--) {
        str[i + pad_count] = str[i];
    }

    for (uint16_t i = 0; i < pad_count; i++) {
        str[i] = pad;
    }
}