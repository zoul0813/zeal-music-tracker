#include <stdint.h>
#include <zos_keyboard.h>
#include <zos_sys.h>
#include <windows.h>
#include <keyboard.h>

#include "shared.h"
#include <zgdk/sound/tracker.h>
#include "file_dialog.h"

#define FILE_DIALOG_W (SCREEN_COL80_WIDTH - 10)
#define FILE_NAME_MAX (FILE_DIALOG_W - 19)

window_t win_FileDialog = {
    .x     = 5,
    .y     = 10,
    .w     = FILE_DIALOG_W,
    .h     = 5,
    .flags = WIN_BORDER | WIN_SHADOW,
    .fg    = TEXT_COLOR_BLACK,
    .bg    = TEXT_COLOR_LIGHT_GRAY,
    .fg_highlight = TEXT_COLOR_WHITE,
    .title = "Save As...",
};

uint8_t get_filename(char *buff) {
    uint8_t x = 13, cx = win_FileDialog.x + x + 1;
    uint8_t y = 2, cy = win_FileDialog.y + y;
    cursor_xy(cx, cy);
    cursor(1);
    window_gotox(&win_FileDialog, x);

    uint8_t pos = 0;
    while(1) {
        unsigned char key = getkey();
        if(key == 0) continue;

        switch(key) {
            case KB_KEY_ENTER: {
                // done
                buff[pos] = 0x00;
                goto end_loop;
            } break;
            case KB_ESC: {
                cursor(0);
                return 0;
            } break;
            case KB_KEY_BACKSPACE:
            case KB_DELETE: {
                if (pos == 0)
                    break;
                pos--;
                buff[pos] = 0x00;
                cx--;
                cursor_x(cx);
                x--;
                window_gotox(&win_FileDialog, x);
                window_putc(&win_FileDialog, CH_DOT);
                window_gotox(&win_FileDialog, x);
            } break;
            default: {
                if (pos >= FILE_NAME_MAX)
                    break;
                char c = getch(key);
                buff[pos] = c;
                pos++;
                cx++;
                cursor_x(cx);
                x++;
                window_putc(&win_FileDialog, c);
            }
        }
    }
end_loop:
    cursor(0);

    return 1;
}

uint8_t file_dialog_show(file_dialog_t type)
{
    zos_err_t err = ERR_SUCCESS;
    uint8_t refresh_view = 0;
    
    switch (type) {
        case FILE_SAVE: {
            win_FileDialog.title = "Save As";
        } break;
        case FILE_LOAD: {
            win_FileDialog.title = "Load From";
        } break;
    }
    dialog_open(&win_FileDialog);
    window_puts(&win_FileDialog, "\n  Filename: [");
    for(uint8_t i = 0; i <= FILE_NAME_MAX; i++) {
        window_putc(&win_FileDialog, CH_DOT);
    }
    window_putc(&win_FileDialog, ']');

    if(!get_filename(textbuff)) goto close_dialog;

    switch (type) {
        case FILE_SAVE: {
            window_title(&win_FileDialog, "Saving...");
            err = zmt_file_save(&track, textbuff);
            handle_error(err, "file save", 1);
        } break;
        case FILE_LOAD: {
            window_title(&win_FileDialog, "Loading...");
            err = zmt_file_load(&track, textbuff);
            handle_error(err, "file open", 1);
            if (err == ERR_SUCCESS)
                refresh_view = 1;
        } break;
    }
    if (err == ERR_SUCCESS)
        dirty_track = 0;

close_dialog:
    window_restore();
    return refresh_view;
}
