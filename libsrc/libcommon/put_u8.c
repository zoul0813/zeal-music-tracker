#include <stdint.h>
#include <zos_errors.h>
#include <zos_sys.h>
#include <zos_vfs.h>
#include "common.h"

uint16_t put_u8(uint8_t i) {
    char c[3];
    u8tohex(i, c, 'A');
    uint16_t size = 3;
    zos_err_t err = write(DEV_STDOUT, &c, &size);
    if(err != ERR_SUCCESS) exit(err);
    return size;
}