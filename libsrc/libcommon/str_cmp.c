#include <stdint.h>

int16_t str_cmp(void* src1, const void* src2) {
    const uint8_t* s1 = src1;
    const uint8_t* s2 = src2;

    for(;;) {
        char d = *s2++;
        signed char ret = *s1++ - d;
        if(ret != 0 || d == '\0')
            return ret;
    }
}