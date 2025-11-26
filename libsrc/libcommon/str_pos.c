#include <stdint.h>

int16_t str_pos(const char* str, char c) {
    if (!str) return -1;
    const char* start = str;
    while (*str) {
        if (*str == c) {
            return (int16_t)(str - start);
        }
        ++str;
    }
    return -1;
}