#include <stdio.h>
#include <stdint.h>
#include <zos_keyboard.h>
#include "windows.h"
#include "keyboard.h"
#include "shared.h"
#include "tracker.h"
#include "file_dialog.h"

window_t win_FileDialog = {
    .x     = 5,
    .y     = 10,
    .w     = SCREEN_COL80_WIDTH - 10,
    .h     = 5,
    .flags = WIN_BORDER | WIN_SHADOW,
    .fg    = TEXT_COLOR_BLACK,
    .bg    = TEXT_COLOR_LIGHT_GRAY,
    .title = "Save As...",
};

file_dialog_t dialog_type = FILE_SAVE;
uint8_t char_index = 0;
uint8_t max_len = 0;

void file_dialog_show(file_dialog_t type)
{
    zos_err_t err = ERR_SUCCESS;
    dialog_type = type;
    char_index = 0;
    switch (type) {
        case FILE_SAVE: {
            win_FileDialog.title = "Save As";
        } break;
        case FILE_LOAD: {
            win_FileDialog.title = "Load From";
        } break;
    }
    window(&win_FileDialog);
    window_puts(&win_FileDialog, "\n  Filename: [");
    max_len = 0;
    for(uint8_t i = 0; i < win_FileDialog.w - 18; i++) {
        window_putc(&win_FileDialog, 0xF9); //\xF9\xF9\xF9\xF9\xF9\xF9\xF9\xF9\xF9\xF9\xF9]
        max_len++;
    }
    max_len--;
    window_putc(&win_FileDialog, ']');
    cursor_xy(win_FileDialog.x + 14 + char_index, win_FileDialog.y + 2);
    cursor(1);
    window_gotox(&win_FileDialog, 13);
    zvb_peri_text_color = ((TEXT_COLOR_LIGHT_GRAY << 4) | (TEXT_COLOR_BLACK & 0x0F));
    err                 = kb_mode((void*) (KB_READ_BLOCK | KB_MODE_COOKED));
    handle_error(err, "keyboard mode", 0);

    uint16_t size = max_len;
    sprintf(textbuff, "            ");
    err = read(DEV_STDIN, textbuff, &size);
    handle_error(err, "keyboard read", 0);
    textbuff[size - 1] = 0x00;

    sprintf(textbuff, "%.12s", textbuff);
    cursor(0);

    err = kb_mode((void*) (KB_READ_NON_BLOCK | KB_MODE_RAW));
    handle_error(err, "init keyboard", 0);
    dirty_track = 0; // either way, we just saved or loaded, so it's clean
    switch (type) {
        case FILE_SAVE: {
            window_title(&win_FileDialog, "Saving...");
            err = zmt_file_save(&track, textbuff);
            handle_error(err, "file save", 0);
        } break;
        case FILE_LOAD: {
            window_title(&win_FileDialog, "Loading...");
            err = zmt_file_load(&track, textbuff);
            handle_error(err, "file open", 0);
        } break;
    }

    if (close_handler != NULL) {
        close_handler();
    }
}

// uint8_t file_keypress_handler(unsigned char key)
// {
//     if((key >= 0x20) && (key <= 0x7E)) {
//         // printable
//         if(char_index < max_len) {
//             textbuff[char_index] = key;
//             text_map_vram();
//             SCR_TEXT[win_FileDialog.y + 2][win_FileDialog.x + 14 + char_index] = key;
//             text_demap_vram();
//             char_index++;
//             cursor_x(win_FileDialog.x + 14 + char_index);
//         }
//         return 1;
//     }
//     switch (key) {
//         case KB_ESC: {
//             if (close_handler != NULL) {
//                 cursor(0);
//                 close_handler();
//             }
//         } break;
//         case KB_KEY_BACKSPACE: {
//             if(char_index == 0) break;
//             text_map_vram();
//             textbuff[char_index] = 0x00;
//             SCR_TEXT[win_FileDialog.y + 2][win_FileDialog.x + 14 + char_index] = ' ';
//             text_demap_vram();
//             char_index--;
//             cursor_x(win_FileDialog.x + 14 + char_index);
//         } break;
//         case KB_KEY_ENTER: {
//             dirty_track = 0; // either way, we just saved or loaded, so it's clean
//             textbuff[char_index] = 0x00;
//             switch (dialog_type) {
//                 case FILE_SAVE: {
//                     window_title(&win_FileDialog, "Saving...");
//                     cursor_xy(0,0);
//                     printf("Save: %s\n", textbuff);
//                     zos_err_t err = zmt_file_save(&track, textbuff);
//                     handle_error(err, "file save", 0);
//                 } break;
//                 case FILE_LOAD: {
//                     window_title(&win_FileDialog, "Loading...");
//                     cursor_xy(0,0);
//                     printf("Load: %s\n", textbuff);
//                     zos_err_t err = zmt_file_load(&track, textbuff);
//                     handle_error(err, "file open", 0);
//                 } break;
//             }
//             if (close_handler != NULL) {
//                 cursor(0);
//                 close_handler();
//             }
//         } break;
//         default: {
//             return 0; // unhandled
//         }
//     }
//     return 1;
// }