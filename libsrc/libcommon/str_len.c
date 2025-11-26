#include <stdint.h>

uint16_t str_len(const char* str)
{
    uint16_t length = 0;
    while (str[length]) length++;
    return length;
}