#include <stdio.h>
#include <string.h>

#include <zos_sys.h>
#include <zos_vfs.h>
#include <zos_errors.h>
#include <zos_keyboard.h>
#include <zos_video.h>
#include <zvb_hardware.h>
#include <zvb_sound.h>

#include "zmt.h"
#include "conio.h"
#include "windows.h"
#include "tracker.h"

static const char dummy[1];
static char textbuff[SCREEN_COL80_WIDTH];

__sfr __banked __at(0x9d) vid_ctrl_status;
const __sfr __banked __at(0xF0) mmu_page0_ro;
__sfr __at(0xF0) mmu_page0;
uint8_t *text_layer1 = (uint8_t *) 0x1000;

pattern_t pattern0;
pattern_t pattern1;
pattern_t pattern2;
pattern_t pattern3;
pattern_t pattern4;
pattern_t pattern5;
pattern_t pattern6;
pattern_t pattern7;

#define PATTERN_WIN_X     5U
#define PATTERN_WIN_Y     3U
#define PATTERN_WIN_WIDTH 15U

window_t winMain = {
  .x = 0,
  .y = 0,
  .w = SCREEN_COL80_WIDTH,
  .h = SCREEN_COL80_HEIGHT,
  .fg = TEXT_COLOR_LIGHT_GRAY,
  .bg = TEXT_COLOR_DARK_GRAY,
  .flags = WIN_BORDER,
  .title = "Zeal Music Tracker",
};

window_t winHelp = {
  .x = PATTERN_WIN_X + (PATTERN_WIN_WIDTH * 0),
  .y = PATTERN_WIN_Y,
  .w = 17 + 2 + 17, // wtf?
  .h = 15,
  .flags = WIN_BORDER,
  .fg = TEXT_COLOR_LIGHT_GRAY,
  .bg = TEXT_COLOR_BROWN,
  .title = "Controls",
};

window_t win_Pattern1 = {
  .x = PATTERN_WIN_X + (PATTERN_WIN_WIDTH * 0),
  .y = PATTERN_WIN_Y,
  .h = 1 + STEPS_PER_PATTERN + 2,
  .w = PATTERN_WIN_WIDTH,
  .fg = VOICE_WINDOW_FG,
  .bg = VOICE_WINDOW_BG,
  .flags = WIN_BORDER,
  .title = "Voice 1"
};

window_t win_Pattern2 = {
  .x = PATTERN_WIN_X + (PATTERN_WIN_WIDTH * 1),
  .y = PATTERN_WIN_Y,
  .h = 1 + STEPS_PER_PATTERN + 2,
  .w = PATTERN_WIN_WIDTH,
  .fg = VOICE_WINDOW_FG,
  .bg = VOICE_WINDOW_BG,
  .flags = WIN_BORDER,
  .title = "Voice 2"
};

window_t win_Pattern3 = {
  .x = PATTERN_WIN_X + (PATTERN_WIN_WIDTH * 2),
  .y = PATTERN_WIN_Y,
  .h = 1 + STEPS_PER_PATTERN + 2,
  .w = PATTERN_WIN_WIDTH,
  .fg = VOICE_WINDOW_FG,
  .bg = VOICE_WINDOW_BG,
  .flags = WIN_BORDER,
  .title = "Voice 3"
};

window_t win_Pattern4 = {
  .x = PATTERN_WIN_X + (PATTERN_WIN_WIDTH * 3),
  .y = PATTERN_WIN_Y,
  .h = 1 + STEPS_PER_PATTERN + 2,
  .w = PATTERN_WIN_WIDTH,
  .fg = VOICE_WINDOW_FG,
  .bg = VOICE_WINDOW_BG,
  .flags = WIN_BORDER,
  .title = "Voice 4"
};

window_t* windows[NUM_VOICES] = {
    &win_Pattern1,
    &win_Pattern2,
    &win_Pattern3,
    &win_Pattern4,
};

track_t track = {
  .title = "Track 1",
  .patterns = {
    &pattern0,
    &pattern1,
    &pattern2,
    &pattern3,
    &pattern4,
    &pattern5,
    &pattern6,
    &pattern7,
  }
};

// track_t* active_track = NULL;
pattern_t *active_pattern = 0;
uint8_t active_pattern_index = 0;
voice_t* active_voice = NULL;
step_t* last_step_edit = NULL;
uint8_t active_voice_index = 0;
uint8_t active_cell = 0;
uint8_t active_step = 0;
uint8_t zmt_last_step = 0;

unsigned char active_dialog = 0x00;

uint8_t cursor_x = 0;
uint8_t cursor_x_offset = 0;
uint8_t cursor_y = (PATTERN_WIN_Y + 2);

playback_state_t state = T_NONE;

#define ACTIVATE_VOICE(index) \
  active_voice_index = index; \
  active_voice = &active_pattern->voices[index]; \
  cursor_x = windows[active_voice_index]->x + 2;

static inline void wait_vblank(void) {
  while((vid_ctrl_status & 2) == 0) { }
}

static inline void wait_end_vblank(void) {
  while(vid_ctrl_status & 2) { }
}

void update_cell(voice_t *voice, int8_t amount) {
  step_t *step = &voice->steps[active_step];
  switch(active_cell) {
    case Cell_Frequency:
      if(amount > 1) amount = 12;
      if(amount < -1) amount = -12;

      if(step->note >= NOTE_OUT_OF_RANGE) {
        if(amount > 0) {
          step->note = NOTE_MIDDLE_C;
        } else {
          step->note = NUM_NOTES - 1;
        }
      } else {
        step->note += amount;
      }
      if(step->note >= NUM_NOTES) {
        step->note = NOTE_OUT_OF_RANGE;
      }
      break;
    case Cell_Waveform:
      if(amount < 0 && step->waveform == 0xFF) {
        step->waveform = 0x04;
      }
      step->waveform += amount;
      if(step->waveform > 0x03) {
        step->waveform = 0xFF;
      }
      break;
    case Cell_Effect1:
      if(amount > 1) amount = 16;
      if(amount < -1) amount = -16;
      step->fx1 += amount;
      break;
    case Cell_Effect2:
      if(amount > 1) amount = 16;
      if(amount < -1) amount = -16;
      step->fx2 += amount;
      break;
  }
}

void color_step(uint8_t step_index, uint8_t color) {
  uint8_t x1 = windows[0]->x + 2;
  uint8_t x2 = windows[1]->x + 2;
  uint8_t x3 = windows[2]->x + 2;
  uint8_t x4 = windows[3]->x + 2;

  uint8_t y = windows[0]->y + 1 + step_index + 1;
  uint8_t bg = windows[0]->bg;

  uint8_t clr = (bg << 4) | (color & 0x0F);

  uint8_t mmu_page_current = mmu_page0_ro;
  __asm__("di");
  mmu_page0 = VID_MEM_PHYS_ADDR_START >> 14;
  // MAGIC: 12 = magic length of the step text :)
  text_layer1[y * SCREEN_COL80_WIDTH + 2] = (winMain.bg << 4) | color;
  text_layer1[y * SCREEN_COL80_WIDTH + 3] = (winMain.bg << 4) | color;
  for(uint8_t i = 0; i < 12; i++) {
      text_layer1[y * SCREEN_COL80_WIDTH + x1 + i] = clr;
      text_layer1[y * SCREEN_COL80_WIDTH + x2 + i] = clr;
      text_layer1[y * SCREEN_COL80_WIDTH + x3 + i] = clr;
      text_layer1[y * SCREEN_COL80_WIDTH + x4 + i] = clr;
  }
  mmu_page0 = mmu_page_current;
  __asm__("ei");
}

void refresh_step(uint8_t voice_index, uint8_t step_index) {
  window_t *w = windows[voice_index];
  voice_t *voice = &active_pattern->voices[voice_index];
  step_t *step = &voice->steps[step_index];

  window_gotoxy(w, 0, step_index + 1);

  if((step_index == 0) || (step_index % 4 == 0)) {
    window_putc(w, 249);
  } else {
    window_putc(w, ' ');
  }

  if(step->note == NOTE_OUT_OF_RANGE) {
    window_puts(w, "---");
  } else {
    sprintf(textbuff, "%.3s", NOTE_NAMES[step->note]);
    // TODO: more optimized?
    // textbuff[0] = NOTE_NAMES[step->note][0];
    // textbuff[1] = NOTE_NAMES[step->note][1];
    // textbuff[2] = NOTE_NAMES[step->note][2];
    // textbuff[3] = ' ';
    // textbuff[4] = ' ';
    // textbuff[5] = 0x00;
    window_puts(w, textbuff);
  }

  if(step->waveform == 0xFF) {
    window_puts(w, " - ");
  } else {
    sprintf(textbuff, " %01X ", step->waveform & 0xF);
    window_puts(w, textbuff);
  }

  if(step->fx1 == 0xFF) {
    window_puts(w, "-- ");
  } else {
    sprintf(textbuff, "%02X ", step->fx1);
    window_puts(w, textbuff);
  }

  if(step->fx2 == 0xFF) {
    window_puts(w, "-- ");
  } else {
    sprintf(textbuff, "%02X ", step->fx2);
    window_puts(w, textbuff);
  }
}

void refresh_steps(uint8_t step_index) {
  if(step_index >= STEPS_PER_PATTERN) return;
  refresh_step(0, step_index);
  refresh_step(1, step_index);
  refresh_step(2, step_index);
  refresh_step(3, step_index);
}

void refresh_track(uint8_t voice_index) {
  // track_t *track = &tracks[track_index];
  voice_t *voice = &active_pattern->voices[voice_index];
  window_t *window = windows[voice_index];
  uint8_t i;
  window_gotoxy(window, 0, 0);
  window_puts_color(window, " No. W F1 F2\n", TEXT_COLOR_WHITE);
  for(i = 0; i < STEPS_PER_PATTERN; i++) {
    refresh_step(voice_index, i);
  }
}

void refresh_help(void) {
  window_gotoxy(&winHelp, 0, 0);
  window_puts(&winHelp, "       Esc: Quit\n");
  window_puts(&winHelp, "       1-4: Change Voice\n");
  window_puts(&winHelp, "     Up/Dn: Change Step\n");
  window_puts(&winHelp, "  Home/End: First/Last\n");
  window_puts(&winHelp, "       Tab: Next Cell\n");
  window_puts(&winHelp, "Left/Right: Edit Step\n");
  window_puts(&winHelp, " PgUp/PgDn: Edit Step 100\n");
  window_puts(&winHelp, "       Ins: Dup. Step\n");
  window_puts(&winHelp, "       Del: Clear Step\n");
  window_puts(&winHelp, "     Space: Play/Stop\n");
}

void refresh(void) {
  active_step = 0;
  window_gotoxy(&winMain, 0, PATTERN_WIN_Y + 1);
  sprintf(textbuff, " P%.1u", active_pattern_index);
  window_puts(&winMain, textbuff);
  for(uint8_t i = 0; i < NUM_VOICES; i++) {
    window_t *w = windows[i];
    window(w);
    refresh_track(i);
  }
  color_step(active_step, VOICE_WINDOW_Hl2);
}

zos_err_t kb_mode(void *arg) {
//  void* arg = (void*) (KB_READ_NON_BLOCK | KB_MODE_RAW);
  return ioctl(DEV_STDIN, KB_CMD_SET_MODE, arg);
}

uint8_t input(void) {
  uint16_t size = sizeof(textbuff);
  zos_err_t err = read(DEV_STDIN, textbuff, &size);
  if (err != ERR_SUCCESS) {
    printf("Failed to read from DEV_STDIN\n");
    exit(err);
  }

  uint8_t i = 0;
  for(i = 0; i < size; i++) {
    const char c = textbuff[i];

    if(c == 0) break;
    if(c == KB_RELEASED) {
      // released = 1;
      i++;
      continue;
    }

    switch(c) {
      case KB_ESC: {
        return ACTION_QUIT;
      } break;
      /** TRANSPORT */
      case KB_KEY_SPACE: {
        if(state == T_NONE) {
          state = T_PLAY;
          zmt_pattern_reset(active_pattern);
          cursor(0);
          /* show the "play" icon */
          _gotoxy(win_Pattern1.x, win_Pattern1.y - 2);
          textcolor(TEXT_COLOR_RED);
          bgcolor(winMain.bg);
          cputc(CH_PLAY);
        } else {
          state = T_NONE;
          // stop sound
          zmt_sound_off();

          /* hide the "play" icon */
          _gotoxy(win_Pattern1.x, win_Pattern1.y - 2);
          textcolor(winMain.fg);
          bgcolor(winMain.bg);
          cputc(' '); // clear the "> nnn" from the frame counter
          color_step(zmt_last_step, VOICE_WINDOW_FG);
          cursor(1);
        }
      } break;
    }

    if(state == T_NONE) {
      switch(c) {
        /* Dialogs */
        case KB_KEY_H: {
          cursor(0);
          if(active_dialog == 0x00) {
            active_dialog = 'h';
            window(&winHelp);
          } else {
            active_dialog = 0x00;
            refresh_track(0);
            refresh_track(1);
            refresh_track(2);
            refresh_track(3);
          }
          cursor(1);
        } break;

        /* Patterns*/
        case KB_KEY_N: {
          if(track.pattern_count < NUM_PATTERNS) {
            track.pattern_count++;
            active_pattern_index = track.pattern_count - 1;
          }
          active_pattern = track.patterns[active_pattern_index];
          zmt_pattern_init(active_pattern);
          ACTIVATE_VOICE(active_voice_index);
          refresh();
        } break;
        case KB_KEY_LEFT_BRACKET: {
          if(active_pattern_index > 0) {
            active_pattern_index--;
          } else {
            active_pattern_index = track.pattern_count - 1;
          }
          active_pattern = track.patterns[active_pattern_index];
          ACTIVATE_VOICE(active_voice_index);
          refresh();
        } break;
        case KB_KEY_RIGHT_BRACKET: {
          if(active_pattern_index < (track.pattern_count - 1)) {
            active_pattern_index++;
          } else {
            active_pattern_index = 0;
          }
          active_pattern = track.patterns[active_pattern_index];
          ACTIVATE_VOICE(active_voice_index);
          refresh();
        } break;

        /* Voices */
        //   return ACTION_QUIT;
        // } break;
        case KB_KEY_1: {
          ACTIVATE_VOICE(0);
        } break;
        case KB_KEY_2: {
          ACTIVATE_VOICE(1);
        } break;
        case KB_KEY_3: {
          ACTIVATE_VOICE(2);
        } break;
        case KB_KEY_4: {
          ACTIVATE_VOICE(3);
        } break;

        /* Steps */
        case KB_DOWN_ARROW: {
          uint8_t old_step = active_step;
          active_step++;
          if(active_step > (STEPS_PER_PATTERN - 1)) active_step = 0;
          color_step(old_step, VOICE_WINDOW_FG);
          color_step(active_step, VOICE_WINDOW_Hl2);
          cursor_y = (PATTERN_WIN_Y + 2) + active_step;
        } break;
        case KB_UP_ARROW: {
          uint8_t old_step = active_step;
          if(active_step > 0) active_step--;
          else active_step = (STEPS_PER_PATTERN - 1);
          color_step(old_step, VOICE_WINDOW_FG);
          color_step(active_step, VOICE_WINDOW_Hl2);
          cursor_y = (PATTERN_WIN_Y + 2) + active_step;
        } break;
        case KB_HOME: {
          uint8_t old_step = active_step;
          active_step = 0;
          color_step(old_step, VOICE_WINDOW_FG);
          color_step(active_step, VOICE_WINDOW_Hl2);
          cursor_y = (PATTERN_WIN_Y + 2) + active_step;
        } break;
        case KB_END: {
          uint8_t old_step = active_step;
          active_step = (STEPS_PER_PATTERN - 1);
          color_step(old_step, VOICE_WINDOW_FG);
          color_step(active_step, VOICE_WINDOW_Hl2);
          cursor_y = (PATTERN_WIN_Y + 2) + active_step;
        } break;

        /* Cells */
        case KB_KEY_TAB: {
          active_cell++;
          if(active_cell > 3) active_cell = 0;
          switch(active_cell) {
            case 0: cursor_x_offset = CELL_OFFSET_FREQ; break;
            case 1: cursor_x_offset = CELL_OFFSET_WAVE; break;
            case 2: cursor_x_offset = CELL_OFFSET_F1; break;
            case 3: cursor_x_offset = CELL_OFFSET_F2; break;
          }
        } break;

        /* Editing */
        case KB_RIGHT_ARROW: {
          update_cell(active_voice, 1);
          last_step_edit = &active_pattern->voices[active_voice_index].steps[active_step];
          // refresh_step(active_voice_index, active_step);
        } break;
        case KB_LEFT_ARROW: {
          update_cell(active_voice, -1);
          last_step_edit = &active_pattern->voices[active_voice_index].steps[active_step];
          // refresh_step(active_voice_index, active_step);
        } break;
        case KB_PG_UP: {
          update_cell(active_voice, 2);
          last_step_edit = &active_pattern->voices[active_voice_index].steps[active_step];
          // refresh_step(active_voice_index, active_step);
        } break;
        case KB_PG_DOWN: {
          update_cell(active_voice, -2);
          last_step_edit = &active_pattern->voices[active_voice_index].steps[active_step];
          // refresh_step(active_voice_index, active_step);
        } break;
        case KB_INSERT: {
          // copy last_step_edit
          active_pattern->voices[active_voice_index].steps[active_step].note = last_step_edit->note;
          active_pattern->voices[active_voice_index].steps[active_step].waveform = last_step_edit->waveform;
          active_pattern->voices[active_voice_index].steps[active_step].fx1 = last_step_edit->fx1;
          active_pattern->voices[active_voice_index].steps[active_step].fx2 = last_step_edit->fx2;
          // refresh_step(active_voice_index, active_step);
        } break;
        case KB_DELETE: {
          // delete current step
          active_pattern->voices[active_voice_index].steps[active_step].note = NOTE_OUT_OF_RANGE;
          active_pattern->voices[active_voice_index].steps[active_step].waveform = WAVEFORM_OUT_OF_RANGE;
          active_pattern->voices[active_voice_index].steps[active_step].fx1 = FX_OUT_OF_RANGE;
          active_pattern->voices[active_voice_index].steps[active_step].fx2 = FX_OUT_OF_RANGE;
          // refresh_step(active_voice_index, active_step);
        } break;
      }
    }
  }
  return 0;
}

int main(int argc, char** argv) {
  zos_err_t err;
  uint8_t i = 0;

  err = kb_mode((void *)(KB_READ_NON_BLOCK | KB_MODE_RAW));
  if(err != ERR_SUCCESS) {
    printf("failed to init keyboard, %d (%02x)\n", err, err);
    exit(err);
  }

  active_pattern = track.patterns[0];
  active_pattern_index = 0;
  if(argc == 1) {
    err = zmt_file_load(&track, argv[0]);
    if (err != ERR_SUCCESS) {
      printf("Failed to load data file: %d\n", err);
      exit(err);
    }
  } else {
    track.pattern_count = 1;
    zmt_pattern_init(active_pattern);
  }

  // zmt_file_save(&track, "rle.zmt");
  // exit(1);

  cursor(0);
  window(&winMain);

  // pattern step indicators
  // textcolor(winMain.fg);
  // bgcolor(winMain.bg);
  window_gotoxy(&winMain, 0, PATTERN_WIN_Y + 1);
  for(i = 0; i <= FX_GOTO_31 - FX_GOTO_0; i++) {
    sprintf(textbuff, " %02X\n", i + FX_GOTO_0);
    window_puts(&winMain, textbuff);
  }

  // TODO: hide/show this on F1?
  // window(&winHelp);
  // refresh_help();

  refresh();

  state = T_NONE;

  zmt_reset(VOL_75);

  ACTIVATE_VOICE(0);
  last_step_edit = &active_pattern->voices[0].steps[active_step];
  cursor(1);

  while(1) {
    wait_vblank();
    TSTATE_LOG(1);
    uint8_t action = input();
    switch(action) {
      case ACTION_QUIT:
        goto __exit;
    }
    switch(state) {
      case T_PLAY: {
        color_step(zmt_last_step, VOICE_WINDOW_FG);
        zmt_last_step = zmt_tick(active_pattern);
        color_step(zmt_last_step, VOICE_WINDOW_HL1);
      } break;
      case T_NONE: {
        refresh_step(active_voice_index, active_step);
        _gotoxy(cursor_x + cursor_x_offset, cursor_y);
      } break;
    }
    TSTATE_LOG(1);
    wait_end_vblank();
  }

__exit:
  zmt_reset(VOL_0);

  // reset the screen
  err = ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
  if(err != ERR_SUCCESS) {
    printf("error reseting screen, %d (%02x)\n", err, err);
    exit(err);
  }

  kb_mode((void*) (KB_READ_BLOCK | KB_MODE_COOKED));
  if(err != ERR_SUCCESS) {
    printf("Failed to change keyboard mode %d (%02x)\n", err, err);
    exit(1);
  }

  uint16_t size = 0;
  do {
    printf("Enter filename to save recording, press enter to quit without saving:\n");
    size = sizeof(textbuff);
    err = read(DEV_STDIN, textbuff, &size);
    if(err != ERR_SUCCESS) {
        printf("keyboard error: %d (%02x)\n", err, err);
        break;
    }

    if(size > 0) {
      switch(textbuff[0]) {
        case KB_KEY_ENTER:
          printf("File not saved\n");
          break;
        default:
          textbuff[size - 1] = 0x00;
          err = zmt_file_save(&track, textbuff);
          if(err != ERR_SUCCESS) {
            printf("error saving file, %d (%02x)\n", err, err);
            exit(err);
          }
          goto __final;
      }
    }
  } while(size == 0);

__final:
  return err;
}