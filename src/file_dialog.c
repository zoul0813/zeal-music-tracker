#include <stdio.h>
#include <stdint.h>
#include <zos_keyboard.h>
#include "windows.h"
#include "keyboard.h"
#include "shared.h"
#include "tracker.h"
#include "file_dialog.h"

window_t win_FileDialog = {
  .x = 10,
  .y = 10,
  .w = 30,
  .h = 5,
  .flags = WIN_BORDER | WIN_SHADOW,
  .fg = TEXT_COLOR_BLACK,
  .bg = TEXT_COLOR_LIGHT_GRAY,
  .title = "Save As...",
};

void file_dialog_show(file_dialog_t type) {
  zos_err_t err = ERR_SUCCESS;
  switch(type) {
    case FILE_SAVE: {
      win_FileDialog.title = "Save As";
    } break;
    case FILE_LOAD: {
      win_FileDialog.title = "Load From";
    } break;
  }
  win_FileDialog.title = "Load From";
  window(&win_FileDialog);
  window_puts(&win_FileDialog, "\n  Filename: [\xF9\xF9\xF9\xF9\xF9\xF9\xF9\xF9\xF9\xF9\xF9\xF9]");
  cursor_xy(win_FileDialog.x + 14, win_FileDialog.y + 2);
  cursor(1);
  zvb_peri_text_color = ((TEXT_COLOR_LIGHT_GRAY << 4) | (TEXT_COLOR_BLACK & 0x0F));
  err = kb_mode((void *)(KB_READ_BLOCK | KB_MODE_COOKED));
  handle_error(err, "keyboard mode", 0);

  uint16_t size = TRACKER_TITLE_LEN;
  sprintf(textbuff, "            ");
  err = read(DEV_STDIN, textbuff, &size);
  handle_error(err, "keyboard read", 0);
  textbuff[size-1] = 0x00;

  sprintf(textbuff, "%.12s", textbuff);
  cursor(0);

  err = kb_mode((void *)(KB_READ_NON_BLOCK | KB_MODE_RAW));
  handle_error(err, "init keyboard", 0);
  switch(type) {
    case FILE_SAVE: {
      window_title(&win_FileDialog, "Saving...");
      err = zmt_file_save(&track, textbuff);
    } break;
    case FILE_LOAD: {
      window_title(&win_FileDialog, "Loading...");
      err = zmt_file_load(&track, textbuff);
    } break;
  }
  handle_error(err, "file open", 0);

  if(close_handler != NULL) close_handler();
}