#include <stdint.h>
#include <zos_errors.h>
#include <zos_sys.h>

#include "common.h"

uint16_t put_sn(const char* str, uint16_t size) {
    uint16_t l = str_len(str);
    uint16_t s = l;
    if(l > size) s = size;
    zos_err_t err = write(DEV_STDOUT, str, &s);
    if(err != ERR_SUCCESS) exit(err);
    return size;
}