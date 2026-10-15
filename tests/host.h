#pragma once
#include <stdint.h>
#include <stdio.h>
#undef SEEK_SET
#undef SEEK_CUR
#undef SEEK_END
#define __SDCC_VERSION_MAJOR 4
#define __SDCC_VERSION_MINOR 4
#define __sdcccall(x)
#define __at(x)
#define __sfr extern uint8_t
#define __banked
#define __critical
