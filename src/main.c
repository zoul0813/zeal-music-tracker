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
#include "sound.h"

static const char dummy[1];
static char textbuff[SCREEN_COL80_WIDTH];

__sfr __banked __at(0x9d) vid_ctrl_status;
const __sfr __banked __at(0xF0) mmu_page0_ro;
__sfr __at(0xF0) mmu_page0;
uint8_t *text_layer1 = (uint8_t *) 0x1000;

typedef struct {
  char title[TRACKER_TITLE_LEN + 1];
  window_t* windows[NUM_VOICES];
  uint8_t pattern_count;
  pattern_t* patterns[NUM_PATTERNS];
} track_t;

// voice_t voice1 = { .index = 0 };
// voice_t voice2 = { .index = 1 };
// voice_t voice3 = { .index = 2 };
// voice_t voice4 = { .index = 3 };

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

track_t track = {
  .title = "Track 1",
  .windows = {
    &win_Pattern1,
    &win_Pattern2,
    &win_Pattern3,
    &win_Pattern4,
  },
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
uint8_t play_next_step = 0;
uint8_t play_last_step = 0;
uint16_t frames = 0;
unsigned char active_dialog = 0x00;

uint8_t cursor_x = 0;
uint8_t cursor_x_offset = 0;
uint8_t cursor_y = (PATTERN_WIN_Y + 2);

playback_state_t state = T_NONE;

#define ACTIVATE_VOICE(index) \
  active_voice_index = index; \
  active_voice = &active_pattern->voices[index]; \
  cursor_x = track.windows[active_voice_index]->x + 2;

#define MAP_SOUND()   zvb_map_peripheral(ZVB_PERI_SOUND_IDX)
#define MAP_TEXT()    zvb_map_peripheral(ZVB_PERI_TEXT_IDX)

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
  uint8_t x1 = track.windows[0]->x + 2;
  uint8_t x2 = track.windows[1]->x + 2;
  uint8_t x3 = track.windows[2]->x + 2;
  uint8_t x4 = track.windows[3]->x + 2;

  uint8_t y = track.windows[0]->y + 1 + step_index + 1;
  uint8_t bg = track.windows[0]->bg;

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
  window_t *w = track.windows[voice_index];
  voice_t *voice = &active_pattern->voices[voice_index];
  step_t *step = &voice->steps[step_index];

  window_gotoxy(w, 0, step_index + 1);

  // // TODO: can this be done once outside of the function?
  // uint8_t clr = w->fg;
  // if(step_index == active_step) {
  //   clr = VOICE_WINDOW_Hl2;
  // }
  // if(state == T_PLAY && step_index == play_next_step) {
  //   clr = VOICE_WINDOW_HL1;
  // }

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

  // color_step(step_index, clr);
}

void refresh_steps(uint8_t step_index) {
  if(step_index >= STEPS_PER_PATTERN) return;
  refresh_step(0, step_index);
  refresh_step(1, step_index);
  refresh_step(2, step_index);
  refresh_step(3, step_index);
}

static inline void process_fx(step_t *step, fx_t fx, sound_voice_t voice) {
  (void *)voice; // TODO: per channel effects
  // pattern_t *pattern = active_pattern; // TODO: gonna have more than one pattern soon, lol

  if((fx >= FX_GOTO_0) && (fx <= FX_GOTO_31)) {
    play_next_step = (fx - FX_GOTO_0) - 1;
    frames = (play_next_step % 16) - 1; // TODO: yeah?
    return;
  }

  switch(fx) {
    case FX_NOTE_OFF: { } break;
    case FX_NOTE_ON: { } break;

    case FX_VOICE_SQ:   // fall thru
    case FX_VOICE_TRI:  // fall thru
    case FX_VOICE_SAW:  // fall thru
    case FX_VOICE_NOISE: {
      SOUND_WAVE(fx);
    } break;

    case FX_COUNT_0: // fall thru
    case FX_COUNT_1: // fall thru
    case FX_COUNT_2: // fall thru
    case FX_COUNT_3: // fall thru
    case FX_COUNT_4: // fall thru
    case FX_COUNT_5: // fall thru
    case FX_COUNT_6: // fall thru
    case FX_COUNT_7: // fall thru
    case FX_COUNT_8: {
      if(step->fx1_attr == 0x00) step->fx1_attr = (fx - 0xC0);
      else step->fx1_attr--;
    } break;

    // TODO: per channel volume control?
    case FX_VOL_00_0: {
      SOUND_VOL(VOL_0);
    } break;
    case FX_VOL_12_5: {
      // TODO: 8 step volume control
      SOUND_VOL(VOL_0);
    } break;
    case FX_VOL_25_0: {
      SOUND_VOL(VOL_25);
    } break;
    case FX_VOL_37_5: {
      // TODO: 8 step volume control
      SOUND_VOL(VOL_25);
    } break;
    case FX_VOL_50_0: {
      SOUND_VOL(VOL_50);
    } break;
    case FX_VOL_62_5: {
      // TODO: 8 step volume control
      SOUND_VOL(VOL_50);
    } break;
    case FX_VOL_75_0: {
      SOUND_VOL(VOL_75);
    } break;
    case FX_VOL_87_5: {
      // TODO: 8 step volume control
      SOUND_VOL(VOL_75);
    } break;
    case FX_VOL_100: {
      SOUND_VOL(VOL_100);
    } break;
  }
}

static inline void play_step(step_t *step, sound_voice_t voice) {
  // pattern_t *pattern = active_pattern; // TODO: gonna have more than one pattern soon, lol

  // FX1 is pre-processed
  if(step->fx1 != FX_OUT_OF_RANGE) process_fx(step, step->fx1, voice);

  SOUND_SELECT(voice);

  if(step->note != NOTE_OUT_OF_RANGE) {
    note_t note = NOTES[step->note];
    SOUND_DIV(note);
  }

  if(step->waveform != WAVEFORM_OUT_OF_RANGE) {
    waveform_t waveform = step->waveform;
    if(waveform > 0x03) waveform = WAV_SQUARE;
    SOUND_WAVE(waveform);
  }

  // only process fx2 if fx1 is not counting, or it has counted down
  if((step->fx1 >= FX_COUNT_0) && (step->fx1 <= FX_COUNT_8) && (step->fx1_attr != 0)) return;
  // FX2 is post-processed ... does this matter?
  if(step->fx2 != FX_OUT_OF_RANGE) process_fx(step, step->fx2, voice);
}

void play_steps(uint8_t step_index) {
  step_t *step1 = &active_pattern->voices[0].steps[step_index];
  step_t *step2 = &active_pattern->voices[1].steps[step_index];
  step_t *step3 = &active_pattern->voices[2].steps[step_index];
  step_t *step4 = &active_pattern->voices[3].steps[step_index];
  MAP_SOUND();
  play_step(step1, VOICE0);
  play_step(step2, VOICE1);
  play_step(step3, VOICE2);
  play_step(step4, VOICE3);
  MAP_TEXT();
}

void pattern_reset(void) {
  frames = 0;
  play_next_step = 0;
  for(uint8_t s = 0; s < STEPS_PER_PATTERN; s++) {
    for(uint8_t v = 0; v < NUM_VOICES; v++) {
      active_pattern->voices[v].steps[s].fx1_attr = 0x00;
      active_pattern->voices[v].steps[s].fx2_attr = 0x00;
    }
  }
}

void refresh_track(uint8_t voice_index) {
  // track_t *track = &tracks[track_index];
  voice_t *voice = &active_pattern->voices[voice_index];
  window_t *window = track.windows[voice_index];
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
    window_t *w = track.windows[i];
    window(w);
    refresh_track(i);
  }
  color_step(active_step, VOICE_WINDOW_Hl2);
}

zos_err_t file_load(const char *filename) {
  zos_dev_t file_dev = open(filename, O_RDONLY);
  if(file_dev < 0) {
    printf("failed to open file, %d (%02x)\n", -file_dev, -file_dev);
    return -file_dev;
  } else {
    zos_err_t err;
    uint16_t size = 0;

    printf("Loading '%s' ...\n", filename);

    size = 3;
    err = read(file_dev, textbuff, &size); // format header
    if(err != ERR_SUCCESS) {
      printf("error reading format header, %d (%02x)\n", err, err);
      return err;
    }
    printf("Format: %03s\n", textbuff);

    size = sizeof(uint8_t);
    err = read(file_dev, textbuff, &size); // version header
    if(err != ERR_SUCCESS) {
      printf("error reading version header, %d (%02x)\n", err, err);
      return err;
    }
    printf("Version: %d\n", textbuff[0]);

    size = TRACKER_TITLE_LEN;
    err = read(file_dev, textbuff, &size); // track title
    if(err != ERR_SUCCESS) {
      printf("error reading track title, %d (%02x)\n", err, err);
      return err;
    }
    memcpy(track.title, textbuff, size);
    track.title[TRACKER_TITLE_LEN] = 0x00; // NUL
    printf("Track: %12s (read: %d)\n", track.title, size);

    size = sizeof(uint8_t);
    err = read(file_dev, &track.pattern_count, &size); // pattern count
    if(err != ERR_SUCCESS) {
      printf("error reading pattern count, %d (%02x)\n", err, err);
      return err;
    }
    printf("Patterns: %d\n", track.pattern_count);

    for(uint8_t p = 0; p < track.pattern_count; p++) {
      printf("Loading pattern %d\n", p);
      err = pattern_load(track.patterns[p], file_dev);
      if(err != ERR_SUCCESS) {
        printf("error loading patterns, %d (%02x)\n", err, err);
        return err;
      }
    }

    printf("File loaded.\n\n");

    close(file_dev);
  }
  return ERR_SUCCESS;
}

zos_err_t file_save(const char *filename) {
  zos_dev_t file_dev = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
  if(file_dev < 0) {
    printf("failed to open file for savings, %d (%02x)", -file_dev, -file_dev);
    return -file_dev;
  } else {
    printf("Saving '%s' ...\n", filename);
    zos_err_t err;
    uint16_t size = 0;

    size = 3;
    err = write(file_dev, "ZMT", &size); // format header
    if(err != ERR_SUCCESS) {
      printf("error saving format header, %d (%02x)\n", err, err);
      exit(err);
    }

    size = sizeof(uint8_t);
    textbuff[0] =  0;
    err = write(file_dev, textbuff, &size); // version header
    if(err != ERR_SUCCESS) {
      printf("error saving version header, %d (%02x)\n", err, err);
      exit(err);
    }

    size = TRACKER_TITLE_LEN;
    sprintf(textbuff, "%-.12s", track.title);
    err = write(file_dev, textbuff, &size); // track title
    if(err != ERR_SUCCESS) {
      printf("error saving title length, %d (%02x)\n", err, err);
      exit(err);
    }

    size = sizeof(uint8_t);
    err = write(file_dev, &track.pattern_count, &size); // pattern count
    if(err != ERR_SUCCESS) {
      printf("error saving pattern count, %d (%02x)\n", err, err);
      exit(err);
    }

    for(uint8_t p = 0; p < track.pattern_count; p++) {
      printf("Writing pattern %d\n", p);
      err = pattern_save(track.patterns[p], file_dev);
      if(err != ERR_SUCCESS) {
      printf("error saving patterns, %d (%02x)\n", err, err);
      exit(err);
    }
    }
    printf("File saved.\n");
    close(file_dev);
  }
  return ERR_SUCCESS;
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
          pattern_reset();
          cursor(0);
          /* show the "play" icon */
          _gotoxy(win_Pattern1.x, win_Pattern1.y - 2);
          textcolor(TEXT_COLOR_RED);
          bgcolor(winMain.bg);
          cputc(CH_PLAY);
        } else {
          state = T_NONE;
          // stop sound
          MAP_SOUND();
          SOUND_OFF();
          MAP_TEXT();
          /* hide the "play" icon */
          _gotoxy(win_Pattern1.x, win_Pattern1.y - 2);
          textcolor(winMain.fg);
          bgcolor(winMain.bg);
          cputc(' '); // clear the "> nnn" from the frame counter
          color_step(play_next_step, VOICE_WINDOW_FG);
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
          pattern_init(active_pattern);
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
    err = file_load(argv[0]);
    if (err != ERR_SUCCESS) {
      printf("Failed to load data file: %d\n", err);
      exit(err);
    }
  } else {
    track.pattern_count = 1;
    pattern_init(active_pattern);
  }

  // file_save("rle.zmt");
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

  MAP_SOUND();
  SOUND_RESET(VOL_75);
  MAP_TEXT();

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
        if(frames > FRAMES_PER_QUARTER) frames = 0;
        // if(frames % FRAMES_PER_EIGTH == 0) { }
        if(frames % FRAMES_PER_SIXTEENTH == 0) {
          color_step(play_last_step, VOICE_WINDOW_FG);
          play_last_step = play_next_step;

          color_step(play_next_step, VOICE_WINDOW_HL1);
          play_steps(play_next_step);

          play_next_step++;
          if(play_next_step >= STEPS_PER_PATTERN) play_next_step = 0;
        }
        frames++;
      } break;
      case T_NONE: {
        // DEBUG
        // sprintf(textbuff, "%2u %2u %2u", cursor_x, cursor_x_offset, cursor_y);
        // window_gotoxy(&winMain, 0, 0);
        // window_puts(&winMain, textbuff);

        refresh_step(active_voice_index, active_step);
        _gotoxy(cursor_x + cursor_x_offset, cursor_y);
      } break;
    }
    TSTATE_LOG(1);
    wait_end_vblank();
  }

__exit:
  MAP_SOUND();
  SOUND_RESET(VOL_0);
  MAP_TEXT();

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
          err = file_save(textbuff);
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