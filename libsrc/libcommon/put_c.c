#include <stdint.h>
#include <zos_errors.h>
#include <zos_vfs.h>
#include <zos_sys.h>

uint16_t put_c(const char c) {
    uint16_t size = 1;
    zos_err_t err = write(DEV_STDOUT, &c, &size);
    if(err != ERR_SUCCESS) exit(err);
    return size;
}