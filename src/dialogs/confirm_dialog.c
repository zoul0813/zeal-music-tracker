#include <stdint.h>
#include <zos_keyboard.h>
#include <windows.h>
#include <dialogs.h>

#include "shared.h"
#include "strings.h"
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
    .fg_highlight = TEXT_COLOR_DARK_RED,
    .title = STR_DIALOG_CONFIRM,
};

keypress_t keypress_handler_backup                = NULL;
current_step_t current_step_handler_backup        = NULL;
current_step_t current_arrangement_handler_backup = NULL;

static uint8_t is_notice;
static uint8_t message_keypress_handler(unsigned char key);

static const dialog_action_t confirm_actions[] = {
    { "Yes", KB_KEY_Y, 0, 0 },
    { "No",  KB_KEY_N, 0, 0 },
};

static const dialog_action_t notice_actions[] = {
    { "Close", KB_KEY_ENTER, DIALOG_NO_HIGHLIGHT, DIALOG_ACTION_LABEL_BOX },
};

static const dialog_button_style_t message_button_style = {
    .fg        = PATTERN_WINDOW_FG,
    .bg        = PATTERN_WINDOW_BG,
    .highlight = PATTERN_WINDOW_HL1,
    .shadow    = TEXT_COLOR_BLACK,
};

static dialog_t message_dialog = {
    .window         = &win_Confirm,
    .message_x      = 2,
    .message_y      = 1,
    .actions_y      = 3,
    .action_spacing = 2,
    .button_style   = &message_button_style,
};

static void message_dialog_show(const char* title, const char* message,
                                const dialog_action_t* actions, uint8_t action_count)
{
    keypress_handler_backup            = keypress_handler;
    current_step_handler_backup        = current_step_handler;
    current_arrangement_handler_backup = current_arrangement_handler;
    keypress_handler                   = &message_keypress_handler;

    win_Confirm.title = title;
    message_dialog.message = message;
    message_dialog.actions = actions;
    message_dialog.action_count = action_count;
    window_save();
    dialog_draw(&message_dialog);
}

void confirm_dialog_show(const char* message)
{
    if (confirm_handler == NULL)
        return; // ignore the request
    if (close_handler == NULL)
        return; // ignore the request

    is_notice = 0;
    message_dialog_show(STR_DIALOG_CONFIRM, message, confirm_actions, 2);
}

void notice_dialog_show(const char* message)
{
    is_notice = 1;
    message_dialog_show("Notice", message, notice_actions, 1);
}

void reset_handlers(void)
{
    keypress_handler            = keypress_handler_backup;
    current_step_handler        = current_step_handler_backup;
    current_arrangement_handler = current_arrangement_handler_backup;
}

static uint8_t message_keypress_handler(unsigned char key)
{
    uint8_t result = dialog_handle_key(&message_dialog, key);
    if (result == DIALOG_RESULT_NONE)
        return 1;

    reset_handlers();
    window_restore();

    if (is_notice) {
        return 1;
    }

    if (result == 0 && confirm_handler != NULL) {
        confirm_handler(ERR_SUCCESS);
    } else if (result == 1 && active_view == VIEW_QUIT && close_handler != NULL) {
        close_handler();
    }
    return 1;
}
