#ifndef INPUT_H
#define INPUT_H

#include <zos_vfs.h>
#include <zos_keyboard.h>

typedef struct {
    uint8_t key;
    uint8_t released;
    uint8_t repeated;
} input_event_t;

zos_err_t input_init(void);
/* key is zero when no event is pending. Every buffered event is retained. */
zos_err_t input_next(input_event_t* event);
uint8_t input_getkey(void);
uint8_t input_getch(uint8_t key);

#endif
