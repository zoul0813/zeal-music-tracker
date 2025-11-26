#include <stdint.h>
#include "common.h"

uint16_t put_s(const char* str) {
    return put_sn(str, UINT16_MAX);
}
