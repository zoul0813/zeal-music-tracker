#include <zos_vfs.h>
#include <zos_sys.h>
#include "keyboard.h"

// #include "common/shared.h"

static uint8_t kb_buffer[32];
static kb_modifiers_t kb_mod = KB_MOD_NONE;
static const char shifted_digits[] = ")!@#$%^&*(";

zos_err_t kb_mode(void* arg)
{
    return ioctl(DEV_STDIN, KB_CMD_SET_MODE, arg);
}


unsigned char shifted(unsigned char c)
{
    uint8_t shift = (kb_mod & KB_MOD_SHIFT);
    uint8_t caps  = (kb_mod & KB_MOD_CAPS);
    if (!caps && !shift)
        return c;

    if (c >= 'a' && c <= 'z') {
        return c - 0x20;
    }

    if (!shift)
        return c;

    if (c >= '0' && c <= '9') {
        return shifted_digits[c - '0'];
    }

    switch (c) {
        case '\'': // 39
            return '"';
        case ',': // 44
            return '<';
        case '-': // 45
            return '_';
        case '.': // 46
            return '>';
        case '/': // 47
            return '?';
        case ';': // 59
            return ':';
        case '=': // 61
            return '+';
        case '[': // 91
            return '{';
        case '\\': // 92
            return '|';
        case ']': // 93
            return '}';
        case '`': // 96
            return '~';
    }

    return c; // just in case
}

kb_keys_t getkey(void)
{
    uint8_t i;
    uint16_t size;
    zos_err_t err;
    kb_keys_t key;

    size = sizeof(kb_buffer);
    err  = read(DEV_STDIN, kb_buffer, &size);
    if(err) return 0; // TODO: how to capture this from the lib perspective?
    // handle_error(err, "read DEV_STDIN", 1);

    uint8_t rel = 0;
    for (i = 0; i < size; i++) {
        key = kb_buffer[i];

        if (key == 0)
            break; // no key

        if (key == KB_RELEASED) {
            i++;
            key = kb_buffer[i];
            rel = 1;
        }

        switch (key) {
            case KB_CAPS_LOCK: {
                if(rel) kb_mod &= ~KB_MOD_CAPS;
                else kb_mod |= KB_MOD_CAPS;
            } break;
            case KB_LEFT_SHIFT:
            case KB_RIGHT_SHIFT: {
                if(rel) kb_mod &= ~KB_MOD_SHIFT;
                else kb_mod |= KB_MOD_SHIFT;
            } break;
            case KB_LEFT_ALT:
            case KB_RIGHT_ALT: {
                if(rel) kb_mod &= ~KB_MOD_ALT;
                else kb_mod |= KB_MOD_ALT;
            } break;
            case KB_LEFT_CTRL:
            case KB_RIGHT_CTRL: {
                if(rel) kb_mod &= ~KB_MOD_META;
                else kb_mod |= KB_MOD_META;
            } break;
            default: {
                if(!rel) return key;
            }
        }
    }

    return 0;
}

unsigned char getch(kb_keys_t key)
{
    return shifted(key);
}

kb_keys_t waitkey(kb_keys_t key)
{
    uint8_t c;
    do {
        c = getkey();
    } while (c != key);
    return c;
}
