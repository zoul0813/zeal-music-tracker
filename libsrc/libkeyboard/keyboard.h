#ifndef LIBKEYBOARD_H
#define LIBKEYBOARD_H

#include <stdint.h>
#include <zos_errors.h>
#include <zos_keyboard.h>

#define kb_mode_non_block_raw() kb_mode((void*) (KB_READ_NON_BLOCK | KB_MODE_RAW))
#define kb_mode_default()       kb_mode((void*) (KB_READ_BLOCK | KB_MODE_COOKED))


#define ASCII_NULL 0x00
#define ASCII_SOH  0x01
#define ASCII_STX  0x02
#define ASCII_ETX  0x03
#define ASCII_ENQ  0x04
#define ASCII_ACK  0x05
#define ASCII_BEL  0x06
#define ASCII_BS   0x08
#define ASCII_TAB  0x09
#define ASCII_LF   0x0A
#define ASCII_VT   0x0B
#define ASCII_FF   0x0C
#define ASCII_CR   0x0D
#define ASCII_SO   0x0E
#define ASCII_SI   0x0F
#define ASCII_DLE  0x10
#define ASCII_DC1  0x11
#define ASCII_DC2  0x12
#define ASCII_DC3  0x13
#define ASCII_DC4  0x14
#define ASCII_NAK  0x15
#define ASCII_SYN  0x16
#define ASCII_ETB  0x17
#define ASCII_CAN  0x18
#define ASCII_EM   0x19
#define ASCII_SUB  0x1A
#define ASCII_ESC  0x1B
#define ASCII_FS   0x1C
#define ASCII_GS   0x1D
#define ASCII_RS   0x1E
#define ASCII_US   0x1F


typedef enum {
    KB_MOD_NONE  = 0,
    KB_MOD_SHIFT = (1 << 0),
    KB_MOD_CAPS  = (1 << 1),
    KB_MOD_CTRL  = (1 << 2),
    KB_MOD_ALT   = (1 << 3),
    KB_MOD_META  = (1 << 4),
} kb_modifiers_t;

zos_err_t kb_mode(void* arg);
kb_keys_t getkey(void);
unsigned char getch(kb_keys_t c);
kb_keys_t waitkey(kb_keys_t c);

#endif
