#include <stdint.h>

#include "common.h"


int16_t str_ends_with(const char *str, const char *suffix) {
    if(str == NULL || suffix == NULL) return 0;

    size_t s_len = str_len(str);
    size_t suffix_len = str_len(suffix);

    if(suffix_len > s_len) return 0;

    return str_cmp(str + s_len - suffix_len, suffix) == 0;
}