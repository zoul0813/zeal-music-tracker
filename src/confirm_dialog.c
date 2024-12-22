#include <stdint.h>
#include <stdio.h>
#include <zos_keyboard.h>

#include "windows.h"
#include "shared.h"
#include "confirm_dialog.h"

#define CONFIRM_W (SCREEN_COL80_WIDTH / 2)
#define CONFIRM_H 7U
#define CONFIRM_X ((SCREEN_COL80_WIDTH - CONFIRM_W) / 2)
#define CONFIRM_Y ((SCREEN_COL80_HEIGHT - CONFIRM_H) / 2)

window_t win_Confirm = {
    .x     = CONFIRM_X,
    .y     = CONFIRM_Y,
    .w     = CONFIRM_W,
    .h     = CONFIRM_H,
    .flags = WIN_BORDER | WIN_SHADOW,
    .fg    = TEXT_COLOR_BLACK,
    .bg    = TEXT_COLOR_LIGHT_GRAY,
    .title = "Confirm",
};

keypress_t keypress_handler_backup                = NULL;
current_step_t current_step_handler_backup        = NULL;
current_step_t current_arrangement_handler_backup = NULL;

void confirm_dialog_show(const char* message)
{
    if (confirm_handler == NULL)
        return; // ignore the request
    if (close_handler == NULL)
        return; // ignore the request

    keypress_handler_backup            = keypress_handler;
    current_step_handler_backup        = current_step_handler;
    current_arrangement_handler_backup = current_arrangement_handler;
    keypress_handler = &confirm_keypress_handler;

    window(&win_Confirm);
    window_gotoxy(&win_Confirm, 2, 1);
    window_puts(&win_Confirm, message);
    window_puts(&win_Confirm, "\n\n");
    window_puts(&win_Confirm, "  [Y]es  [N]o");
    // window_banner(&win_Confirm, 0, win_Confirm.h - 1, 1, "[\x74Y]es  [\x74N]o");
}

void reset_handlers(void)
{
    keypress_handler            = keypress_handler_backup;
    current_step_handler        = current_step_handler_backup;
    current_arrangement_handler = current_arrangement_handler_backup;
}

uint8_t confirm_keypress_handler(unsigned char key)
{
    switch (key) {
        case KB_KEY_Y: {
            if (confirm_handler != NULL) {
                confirm_handler(ERR_SUCCESS);
            }
        } // fall thru and close???
        case KB_KEY_N: {
            if (close_handler != NULL) {
                reset_handlers();
                close_handler();
            }
        } break;
        default: {
            return 0; // unhandled
        }
    }
    return 1;
}