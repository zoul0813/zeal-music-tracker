#include "input.h"
#include "shared.h"

static uint8_t buffer[32];
static uint16_t position, length;
static uint8_t down[32];
static uint8_t caps_lock;

static uint8_t is_down(uint8_t key)
{
    return down[key >> 3] & (1U << (key & 7));
}

zos_err_t input_init(void)
{
    position = length = 0;
    caps_lock = 0;
    for (uint8_t i = 0; i < sizeof(down); i++)
        down[i] = 0;
    return ioctl(DEV_STDIN, KB_CMD_SET_MODE, (void*)(KB_READ_NON_BLOCK | KB_MODE_RAW));
}

zos_err_t input_next(input_event_t* event)
{
    event->key = event->released = event->repeated = 0;
    if (position == length) {
        length = sizeof(buffer);
        position = 0;
        zos_err_t err = read(DEV_STDIN, buffer, &length);
        if (err != ERR_SUCCESS) {
            length = 0;
            return err;
        }
        if (!length)
            return ERR_SUCCESS;
    }

    uint8_t key = buffer[position++];
    if (key == KB_RELEASED) {
        /* The OS guarantees the release key is in the same read. */
        if (position == length)
            return ERR_FAILURE;
        event->released = 1;
        key = buffer[position++];
    }
    event->key = key;
    event->repeated = !event->released && is_down(key) != 0;
    if (event->released)
        down[key >> 3] &= ~(1U << (key & 7));
    else
        down[key >> 3] |= 1U << (key & 7);
    if (key == KB_CAPS_LOCK && !event->released && !event->repeated)
        caps_lock ^= 1;
    return ERR_SUCCESS;
}

uint8_t input_getkey(void)
{
    input_event_t event;
    zos_err_t err = input_next(&event);
    handle_error(err, "read keyboard", 1);
    if (err || event.released ||
        (event.key >= KB_CAPS_LOCK && event.key <= KB_RIGHT_CTRL))
        return 0;
    return event.key;
}

uint8_t input_getch(uint8_t key)
{
    uint8_t shift = is_down(KB_LEFT_SHIFT) || is_down(KB_RIGHT_SHIFT);
    if (key >= 'a' && key <= 'z')
        return (shift != caps_lock) ? key - 'a' + 'A' : key;
    if (!shift)
        return key;
    if (key >= '0' && key <= '9')
        return ")!@#$%^&*("[key - '0'];
    const char* plain = "`-=[]\\;',./";
    const char* shifted = "~_+{}|:\"<>?";
    for (uint8_t i = 0; plain[i]; i++) {
        if (key == plain[i])
            return shifted[i];
    }
    return key;
}
