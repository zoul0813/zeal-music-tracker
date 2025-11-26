#include <zos_vfs.h>
#include "common.h"

void put_date_part(uint16_t part, char *buffer) {
    u8tohex(part, buffer, 'A');
    put_s(buffer);
}

void put_date(zos_date_t* date, char *buffer) {
    put_date_part(date->d_year_h, buffer);
    put_date_part(date->d_year_l, buffer);
    put_c('-');
    put_date_part(date->d_month, buffer);
    put_c('-');
    put_date_part(date->d_day, buffer);
    put_c(' ');
    put_date_part(date->d_hours, buffer);
    put_c(':');
    put_date_part(date->d_minutes, buffer);
    put_c(':');
    put_date_part(date->d_seconds, buffer);
}