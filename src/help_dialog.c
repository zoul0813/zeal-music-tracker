#include <stdint.h>
#include <stdio.h>
#include <zos_keyboard.h>

#include "windows.h"
#include "shared.h"
#include "help_dialog.h"

#define HELP_W (SCREEN_COL80_WIDTH / 2)
#define HELP_H 26U
#define HELP_X ((SCREEN_COL80_WIDTH - HELP_W) / 2)
#define HELP_Y ((SCREEN_COL80_HEIGHT - HELP_H) / 2)

window_t win_Help = {
    .x = HELP_X,
    .y = HELP_Y,
    .w = HELP_W,
    .h = HELP_H,
    .flags = WIN_BORDER | WIN_SHADOW,
    .fg = TEXT_COLOR_LIGHT_GRAY,
    .bg = TEXT_COLOR_BROWN,
    .title = "Help",
};

void help_dialog_show(void) {
    window(&win_Help);
    window_puts_color(&win_Help, " General\n", COLOR(TEXT_COLOR_WHITE, win_Help.bg));
    window_puts(&win_Help, " \xF9 S - Save File\n");
    window_puts(&win_Help, " \xF9 L - Load File\n");
    window_puts(&win_Help, " \xF9 Space - Play/Stop\n");
    window_puts(&win_Help, " \xF9 Esc - Quit\n");
    window_puts(&win_Help, " \xF9 H - Inline Help\n");
    window_puts(&win_Help, " \xF9 P - Pattern View\n");
    window_puts(&win_Help, " \xF9 A - Arrangement View\n");
    window_puts(&win_Help, "\n");
    window_puts_color(&win_Help, " All Views\n", COLOR(TEXT_COLOR_WHITE, win_Help.bg));
    window_puts(&win_Help, " \xF9 Up/Down - Next Step\n");
    window_puts(&win_Help, " \xF9 Home/End - First/Last Step\n");
    window_puts(&win_Help, " \xF9 Left/Right - Adjust Step\n");
    window_puts(&win_Help, " \xF9 PgUp/PgDown - Adjust Step Plus\n");
    window_puts(&win_Help, " \xF9 Ins - Duplicate Last Step\n");
    window_puts(&win_Help, " \xF9 Del - Delete Step\n");
    window_puts(&win_Help, " \xF9 Tab - Next Cell\n");
    window_puts(&win_Help, "\n");
    // window_puts(&win_Help, " Arrangement View\n");
    window_puts_color(&win_Help, " Pattern View\n", COLOR(TEXT_COLOR_WHITE, win_Help.bg));
    window_puts(&win_Help, " \xF9 1-4 - Voice 1-4\n");
    window_puts(&win_Help, " \xF9 [/] - Prev/Next Pattern\n");
    window_puts(&win_Help, " \xF9 N - New Pattern\n");

    window_gotoxy(&win_Help, (win_Help.w - 18) / 2, win_Help.h - 2);
    window_puts_color(&win_Help, "[ Enter to Close ]", COLOR(TEXT_COLOR_WHITE, win_Help.bg));
}

void help_keypress_handler(unsigned char key) {
    switch (key) {
        case KB_KEY_ENTER: {
            if (close_handler != NULL)
                close_handler();
        } break;
    }
}