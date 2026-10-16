#include <stdint.h>
#include <zos_keyboard.h>
#include <windows.h>

#include "shared.h"
#include "strings.h"
#include "help_dialog.h"

#define HELP_W (SCREEN_COL80_WIDTH / 2)
#define HELP_H 23U
#define HELP_X ((SCREEN_COL80_WIDTH - HELP_W) / 2)
#define HELP_Y ((SCREEN_COL80_HEIGHT - HELP_H) / 2)

window_t win_Help = {
    .x     = HELP_X,
    .y     = HELP_Y,
    .w     = HELP_W,
    .h     = HELP_H,
    .flags = WIN_BORDER | WIN_SHADOW,
    .fg    = TEXT_COLOR_LIGHT_GRAY,
    .bg    = TEXT_COLOR_BROWN,
    .fg_highlight = TEXT_COLOR_WHITE,
    .title = "Help",
};

void help_dialog_show(View view)
{
    switch(view) {
        case VIEW_ARRANGER: win_Help.h = 23; break;
        case VIEW_PATTERN: win_Help.h = 36; break;
    }
    win_Help.y = (SCREEN_COL80_HEIGHT - win_Help.h) / 2;

    dialog_open(&win_Help);
    window_puts_color(&win_Help, " General\n", COLOR(TEXT_COLOR_WHITE, win_Help.bg));
    window_puts(&win_Help, " " CH_BULLET " F2 - Save File\n");
    window_puts(&win_Help, " " CH_BULLET " L - Load File\n");
    window_puts(&win_Help, " " CH_BULLET " Space - Play/Stop (not recording)\n");
    window_puts(&win_Help, " " CH_BULLET " Q - Quit\n");
    window_puts(&win_Help, " " CH_BULLET " F1 - Inline Help\n");
    window_puts(&win_Help, " " CH_BULLET " P - Pattern View\n");
    window_puts(&win_Help, " " CH_BULLET " A - Arrangement (not recording)\n");
    window_puts(&win_Help, "\n");
    window_puts_color(&win_Help, " All Views\n", COLOR(TEXT_COLOR_WHITE, win_Help.bg));
    window_puts(&win_Help, " " CH_BULLET " Up/Down - Next Step\n");
    window_puts(&win_Help, " " CH_BULLET " Home/End - First/Last Step\n");
    window_puts(&win_Help, " " CH_BULLET " Left/Right - Adjust Step\n");
    window_puts(&win_Help, " " CH_BULLET " PgUp/PgDown - Adjust Step Plus\n");
    window_puts(&win_Help, " " CH_BULLET " Ins - Duplicate Last Step\n");
    window_puts(&win_Help, " " CH_BULLET " Del - Delete Step\n");
    window_puts(&win_Help, " " CH_BULLET " Tab - Next Cell\n");
    window_puts(&win_Help, "\n");

    switch (view) {
        case VIEW_ARRANGER: {
            window_puts_color(&win_Help, " Arrangement\n", COLOR(TEXT_COLOR_WHITE, win_Help.bg));
            window_puts(&win_Help, " " CH_BULLET " R,T - Tempo +/-\n");
        } break;
        case VIEW_PATTERN: {
            window_puts_color(&win_Help, " Pattern\n", COLOR(TEXT_COLOR_WHITE, win_Help.bg));
            window_puts(&win_Help, " " CH_BULLET " 1-4 - Voice 1-4\n");
            window_puts(&win_Help, " " CH_BULLET " [/] - Prev/Next Pattern\n");
            window_puts(&win_Help, " " CH_BULLET " F5 - New Pattern\n");
            window_puts(&win_Help, " " CH_BULLET " F6 - Clear Pattern\n");
            window_puts(&win_Help, " " CH_BULLET " F7 - Delete Pattern\n");
            window_puts(&win_Help, " " CH_BULLET " Z X C V B N M - C D E F G A B\n");
            window_puts(&win_Help, " " CH_BULLET " S D G H J - C# D# F# G# A#\n");
            window_puts(&win_Help, " " CH_BULLET " -/= - Octave down/up (default 4)\n");
            window_puts(&win_Help, " " CH_BULLET " R - Record toggle (stop first)\n");
            window_puts(&win_Help, " " CH_BULLET " A - Note OFF when recording\n");
            window_puts(&win_Help, " " CH_BULLET " F - All OFF when recording\n");
            window_puts(&win_Help, " " CH_BULLET " Space - Next row when recording\n");
            window_puts(&win_Help, " " CH_BULLET " Hold keys for chords (4 voices)\n");
            window_puts(&win_Help, " " CH_BULLET " Record changes notes only\n");
        } break;
    }

    window_gotoxy(&win_Help, (win_Help.w - 18) / 2, win_Help.h - 2);
    window_puts_color(&win_Help, STR_ENTER_TO_CLOSE, COLOR(TEXT_COLOR_WHITE, win_Help.bg));
}

uint8_t help_keypress_handler(unsigned char key)
{
    switch (key) {
        case KB_KEY_ENTER: {
            if (close_handler != NULL)
                close_handler();
        } break;
        default: {
            return 1; // Help is modal; do not dispatch global commands.
        }
    }
    return 1;
}
