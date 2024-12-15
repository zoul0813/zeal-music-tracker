#include <stdio.h>
#include <stdint.h>
#include <zos_sys.h>
#include <zos_video.h>

#include "keyboard.h"
#include "windows.h"
#include "tracker.h"
#include "shared.h"
// views
#include "pattern.h"
#include "arrange.h"
#include "file_dialog.h"
static zos_err_t err = ERR_SUCCESS;
unsigned char key = 0;
keypress_t keypress_handler;
current_step_t current_step_handler;
uint8_t mmu_page_current;
uint8_t playing = 0;
uint8_t current_step = 0;
uint8_t current_pattern = 0;

typedef enum {
  VIEW_ARRANGER,
  VIEW_PATTERN,
  VIEW_HELP,
  VIEW_FILE_SAVE,
  VIEW_FILE_LOAD,
} View;

View active_view = VIEW_PATTERN;
View previous_view = VIEW_PATTERN;

window_t win_Main = {
  .x = 0,
  .y = 0,
  .w = SCREEN_COL80_WIDTH,
  .h = SCREEN_COL80_HEIGHT,
  .flags = WIN_BORDER,
  .title = "Zeal Music Tracker",
  .fg = TEXT_COLOR_LIGHT_GRAY,
  .bg = TEXT_COLOR_DARK_GRAY,
};

window_t win_Help = {
  .x = SCREEN_COL80_WIDTH / 4,
  .y = SCREEN_COL80_HEIGHT / 4,
  .w = SCREEN_COL80_WIDTH / 2,
  .h = SCREEN_COL80_HEIGHT / 2,
  .flags = WIN_BORDER | WIN_SHADOW,
  .fg = TEXT_COLOR_WHITE,
  .bg = TEXT_COLOR_BROWN,
  .title = "Help"
};


__sfr __banked __at(0x9d) vid_ctrl_status;
static inline void wait_vblank(void) {
  while((vid_ctrl_status & 2) == 0) { }
}

static inline void wait_end_vblank(void) {
  while(vid_ctrl_status & 2) { }
}


void load_or_init_file(int argc, char** argv) {
  if(argc == 1) {
    err = zmt_file_load(&track, argv[0]);
    handle_error(err, "load data file", 1);
  } else {
    // TODO: init new file
    zmt_track_init(&track);
  }

  printf("Track: %.12s\n", track.title);
}

void view_switch(View view) {
  previous_view = active_view;
  cursor(0);
  switch(view) {
    case VIEW_ARRANGER: {
      window_clrscr(&win_Main);
      keypress_handler = &arrange_keypress_handler;
      current_step_handler = &arrange_current_step_handler;
      arrange_show(0);
    } break;
    case VIEW_PATTERN: {
      window_clrscr(&win_Main);
      keypress_handler = &pattern_keypress_handler;
      current_step_handler = &pattern_current_step_handler;
      pattern_show(current_pattern);
    } break;
    case VIEW_HELP: {
      // move this into a view!
      window(&win_Help);
      window_puts(&win_Help, "Help Text\n");
      window_puts(&win_Help, "Line 2\n");
      window_puts(&win_Help, "Line 3\n");
      window_puts(&win_Help, "Line 4\n");
      window_puts(&win_Help, "Line 5\n");
      window_puts(&win_Help, "Line 6\twith\ttabs\n");
      window_puts(&win_Help, "Line 7a\twi\ttabs\n");
      window_gotoxy(&win_Help, 0, 3);
      window_clreol(&win_Help);
    } break;
    case VIEW_FILE_SAVE: {
      keypress_handler = NULL;
      current_step_handler = NULL;
      file_dialog_show(FILE_SAVE);
    } break;
    case VIEW_FILE_LOAD: {
      keypress_handler = NULL;
      current_step_handler = NULL;
      file_dialog_show(FILE_LOAD);
    }
  }
  active_view = view;
}

void handle_keypress(char key) {
  switch(key) {
    /* QUIT */
    case KB_ESC: __exit(ERR_SUCCESS);
    /* PLAY */
    case KB_KEY_SPACE: {
      playing ^= 1;
      if(playing) {
        // if arranger, reset pattern
        if(active_view == VIEW_ARRANGER) {
          zmt_track_reset(&track, 1);
        } else {
          zmt_track_reset(&track, 0);
        }
      } else {
        zmt_sound_off();
      }
    }; break;
    case KB_KEY_H: {
      view_switch(VIEW_HELP);
    } break;

    /* Files */
    case KB_KEY_S: {
      view_switch(VIEW_FILE_SAVE);
    } break;
    case KB_KEY_L: {
      view_switch(VIEW_FILE_LOAD);
    } break;

    /** VIEWS */
    /* ARRANGER */
    case KB_KEY_A: {
      view_switch(VIEW_ARRANGER);
    } break;
    /* PATTERNS */
    case KB_KEY_P: {
      view_switch(VIEW_PATTERN);
    } break;

    default: {
      // call the current views handler, if one is set
      if(keypress_handler != NULL) {
        keypress_handler(key);
      }
    } break;
  }
}

int main(int argc, char** argv) {
  // initialize the keyboard
  err = kb_mode((void *)(KB_READ_NON_BLOCK | KB_MODE_RAW));
  handle_error(err, "init keyboard", 1);

  // disable cursor blink
  cursor(0);

  // load or init file, and reset ZMT
  load_or_init_file(argc, argv);
  zmt_reset(VOL_75);

  // draw the main window
  window(&win_Main);

  // initialize everything
  current_pattern = 0;
  view_switch(VIEW_ARRANGER);

  // main loop
  while(1) {
    key = getkey();
    handle_keypress(key);
    if(playing) {
      wait_vblank();
      zmt_tick(&track, active_view == VIEW_ARRANGER, &current_pattern, &current_step);
      wait_end_vblank();
      if(current_step_handler != NULL) {
        current_step_handler(current_step);
      }

      // print the playhead
      sprintf(textbuff, "%02X %02X", current_pattern, current_step);
      // window_gotoxy(&win_Main, 0, 0);
      // window_puts(&win_Main, textbuff);
      text_map_vram();
      setcolor(TEXT_COLOR_BLACK, TEXT_COLOR_LIGHT_GRAY);
      cursor_xy(1,1);
      print(textbuff);
      text_demap_vram();
    }
  }

// unreachable
// __final:
//   __exit(ERR_SUCCESS);
//   return 0;
}