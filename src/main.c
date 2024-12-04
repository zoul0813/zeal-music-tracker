#include <stdio.h>
#include <string.h>

#include <zos_sys.h>
#include <zos_vfs.h>
#include <zos_errors.h>
#include <zos_keyboard.h>
#include <zos_video.h>
#include <zvb_gfx.h>
#include <zvb_sound.h>

#include "zmt.h"
#include "conio.h"
#include "windows.h"
#include "tracker.h"

static const char dummy[2];
static char textbuff[SCREEN_COL80_WIDTH];

const __sfr __banked __at(0xF0) mmu_page0_ro;
__sfr __at(0xF0) mmu_page0;
uint8_t *text_layer1 = (uint8_t *) 0x1000;

typedef struct {
  char title[TRACKER_TITLE_LEN + 1];
  window_t* windows[NUM_VOICES];
  pattern_t* pattern;
} track_t;

voice_t voice1 = { .voice = 0 };
voice_t voice2 = { .voice = 1 };
voice_t voice3 = { .voice = 2 };
voice_t voice4 = { .voice = 3 };

pattern_t pattern = {
  .title = "Pattern 1",
  .voices = {
    &voice1,
    &voice2,
    &voice3,
    &voice4,
  },
};

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
  .x = 40,
  .y = 2,
  .w = 17 + 2 + 17, // wtf?
  .h = 15,
  .flags = WIN_BORDER,
  .fg = TEXT_COLOR_LIGHT_GRAY,
  .bg = TEXT_COLOR_BROWN,
  .title = "Controls",
};

#define PATTERN_WIN_X     5U
#define PATTERN_WIN_Y     3U
#define PATTERN_WIN_WIDTH 15U

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

// window_t winDebug = {
//   .x = 2,
//   .y = 2,
//   .h = 5,
//   .w = 25,
//   .fg = TEXT_COLOR_LIGHT_GRAY,
//   .bg = TEXT_COLOR_BLACK,
//   .title = "Debug"
// };

track_t track = {
  .title = "Track 1",
  .windows = {
    &win_Pattern1,
    &win_Pattern2,
    &win_Pattern3,
    &win_Pattern4,
  },
  .pattern = &pattern,
};

// track_t* active_track = NULL;
voice_t* active_voice = NULL;
step_t* last_step_edit = NULL;
uint8_t active_voice_index = 0;
uint8_t active_cell = 0;
uint8_t active_step = 0;
uint8_t play_next_step = 0;
uint8_t play_last_step = 0;
uint16_t frames = 0;

playback_state_t state = T_NONE;

#define ACTIVATE_VOICE(index) \
  active_voice_index = index; \
  active_voice = track.pattern->voices[index];
  // window_gotoxy(track.windows[index], 0, 0);
  // gotoxy(track.windows[index]->x + 2, track.windows[index]->y + 2);

#define MAP_SOUND()   zvb_map_peripheral(ZVB_PERI_SOUND_IDX)
#define MAP_TEXT()    zvb_map_peripheral(ZVB_PERI_TEXT_IDX)

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
      // if(amount < 0) {
      //   if(step->note >= NOTE_OUT_OF_RANGE) {
      //     step->note = NUM_NOTES-1;
      //   }
      // }
      // step->note += amount;
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
  voice_t *voice = track.pattern->voices[voice_index];
  step_t *step = &voice->steps[step_index];

  window_gotoxy(w, 1, step_index + 1);

  // TODO: can this be done once outside of the function?
  uint8_t clr = w->fg;
  if(step_index == active_step) {
    clr = VOICE_WINDOW_Hl2;
  }
  if(state == T_PLAY && step_index == play_next_step) {
    clr = VOICE_WINDOW_HL1;
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

  color_step(step_index, clr);
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
  // pattern_t *pattern = track.pattern; // TODO: gonna have more than one pattern soon, lol

  if((fx >= FX_GOTO_0) && (fx <= FX_GOTO_31)) {
    play_next_step = (fx - FX_GOTO_0) - 1;
    frames = play_next_step % 16; // TODO: yeah?
    return;
  }

  switch(fx) {
    case FX_NOTE_OFF: { } break;
    case FX_NOTE_ON: { } break;

    case FX_COUNT_0: // fall thru
    case FX_COUNT_1: // fall thru
    case FX_COUNT_2: // fall thru
    case FX_COUNT_3: // fall thru
    case FX_COUNT_4: // fall thru
    case FX_COUNT_5: // fall thru
    case FX_COUNT_6: // fall thru
    case FX_COUNT_7: // fall thru
    case FX_COUNT_8: {
      if(step->fx1_attr == 0xFF) step->fx1_attr = (fx - 0xC0);
      else step->fx1_attr--;
    } break;

    // TODO: per channel volume control?
    case FX_VOL_00_0: {
      zvb_sound_set_volume(VOL_0);
    } break;
    case FX_VOL_12_5: {
      // TODO: 8 step volume control
      zvb_sound_set_volume(VOL_0);
    } break;
    case FX_VOL_25_0: {
      zvb_sound_set_volume(VOL_25);
    } break;
    case FX_VOL_37_5: {
      // TODO: 8 step volume control
      zvb_sound_set_volume(VOL_25);
    } break;
    case FX_VOL_50_0: {
      zvb_sound_set_volume(VOL_50);
    } break;
    case FX_VOL_62_5: {
      // TODO: 8 step volume control
      zvb_sound_set_volume(VOL_50);
    } break;
    case FX_VOL_75_0: {
      zvb_sound_set_volume(VOL_75);
    } break;
    case FX_VOL_87_5: {
      // TODO: 8 step volume control
      zvb_sound_set_volume(VOL_75);
    } break;
    case FX_VOL_100: {
      zvb_sound_set_volume(VOL_100);
    } break;
  }
}

static inline void play_step(step_t *step, sound_voice_t voice) {
  // pattern_t *pattern = track.pattern; // TODO: gonna have more than one pattern soon, lol

  // FX1 is pre-processed
  if(step->fx1 != FX_OUT_OF_RANGE) process_fx(step, step->fx1, voice);

  if(step->note != NOTE_OUT_OF_RANGE) {
    note_t note = NOTES[step->note];
    waveform_t waveform = step->waveform;
    if(waveform > 0x03) waveform = WAV_SQUARE;
    waveform &= 03;
    zvb_sound_set_voices(voice, note, waveform);
  }

  // only process fx2 if fx1 is not counting, or it has counted down
  if((step->fx1 >= FX_COUNT_0) && (step->fx1 <= FX_COUNT_8) && (step->fx1_attr != 0)) return;
  // FX2 is post-processed ... does this matter?
  if(step->fx2 != FX_OUT_OF_RANGE) process_fx(step, step->fx2, voice);
}

void play_steps(uint8_t step_index) {
  step_t *step1 = &track.pattern->voices[0]->steps[step_index];
  step_t *step2 = &track.pattern->voices[1]->steps[step_index];
  step_t *step3 = &track.pattern->voices[2]->steps[step_index];
  step_t *step4 = &track.pattern->voices[3]->steps[step_index];
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
      track.pattern->voices[v]->steps[s].fx1_attr = 0x00;
      track.pattern->voices[v]->steps[s].fx2_attr = 0x00;
    }
  }
}

void refresh_track(uint8_t voice_index) {
  // track_t *track = &tracks[track_index];
  voice_t *voice = track.pattern->voices[voice_index];
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

zos_err_t file_load(const char *filename) {
  zos_dev_t file_dev = open(filename, O_RDONLY);
  if(file_dev < 0) {
    printf("failed to open file, %d (%02x)\n", -file_dev, -file_dev);
    pattern_init(track.pattern);
    return -file_dev;
  } else {
    zos_err_t err;
    uint16_t size = 0;
    char text[TRACKER_TITLE_LEN] = { 0x20 };

    size = 3;
    err = read(file_dev, text, &size); // format header
    if(err != ERR_SUCCESS) {
      printf("error reading format header, %d (%02x)\n", err, err);
      return err;
    }
    printf("Format: %03s\n", text);

    size = sizeof(uint8_t);
    err = read(file_dev, text, &size); // version header
    if(err != ERR_SUCCESS) {
      printf("error reading version header, %d (%02x)\n", err, err);
      return err;
    }
    printf("Version: %d\n", text[0]);

    size = TRACKER_TITLE_LEN;
    err = read(file_dev, text, &size); // track title
    if(err != ERR_SUCCESS) {
      printf("error reading track title, %d (%02x)\n", err, err);
      return err;
    }
    memcpy(track.title, text, size);
    track.title[TRACKER_TITLE_LEN] = 0x00; // NUL
    printf("Track: %12s (read: %d)\n", track.title, size);

    size = sizeof(uint8_t);
    err = read(file_dev, text, &size); // pattern count
    if(err != ERR_SUCCESS) {
      printf("error reading pattern count, %d (%02x)\n", err, err);
      return err;
    }
    printf("Patterns: %d\n", text[0]);

    err = pattern_load(track.pattern, file_dev);
    if(err != ERR_SUCCESS) {
      printf("error loading patterns, %d (%02x)\n", err, err);
      return err;
    }

    // for(uint8_t i = 0; i < STEPS_PER_PATTERN; i++) {
    //   printf(
    //     "%05u %01X %02X %02X\n",
    //     track.pattern->voices[0]->steps[i].freq,
    //     track.pattern->voices[0]->steps[i].waveform & 0x0F,
    //     track.pattern->voices[0]->steps[i].fx1,
    //     track.pattern->voices[0]->steps[i].fx2
    //   );
    // }

    close(file_dev);
    // exit(0);
  }
  return ERR_SUCCESS;
}

zos_err_t file_save(const char *filename) {
  zos_dev_t file_dev = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
  if(file_dev < 0) {
    printf("failed to open file for savings, %d (%02x)", -file_dev, -file_dev);
    return -file_dev;
  } else {
    uint16_t size = 0;
    char text[TRACKER_TITLE_LEN] = { 0x20 };

    size = 3;
    write(file_dev, "ZMT", &size); // format header

    size = sizeof(uint8_t);
    text[0] =  0;
    write(file_dev, text, &size); // version header

    size = TRACKER_TITLE_LEN;
    sprintf(text, "%-12s", track.title);
    write(file_dev, text, &size); // track title

    size = sizeof(uint8_t);
    text[0] = 1;
    write(file_dev, text, &size); // pattern count

    pattern_save(track.pattern, file_dev);

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
          cursor(0);
          state = T_PLAY;
          pattern_reset();
          gotoxy(win_Pattern1.x, win_Pattern1.y - 2);
          textcolor(TEXT_COLOR_RED);
          bgcolor(winMain.bg);
          cputc(CH_PLAY);
        } else {
          state = T_NONE;
          // stop sound
          MAP_SOUND();
          zvb_sound_set_voices(VOICEALL, 0, WAV_SQUARE);
          MAP_TEXT();
          gotoxy(win_Pattern1.x, win_Pattern1.y - 2);
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
        // case KB_ESC: {
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

        case KB_DOWN_ARROW: {
          uint8_t old_step = active_step;
          active_step++;
          if(active_step > (STEPS_PER_PATTERN - 1)) active_step = 0;
          color_step(old_step, VOICE_WINDOW_FG);
          // refresh_step(active_voice_index, old_step); // redraw to remove highlight
        } break;
        case KB_UP_ARROW: {
          uint8_t old_step = active_step;
          if(active_step > 0) active_step--;
          else active_step = (STEPS_PER_PATTERN - 1);
          color_step(old_step, VOICE_WINDOW_FG);
          // refresh_step(active_voice_index, old_step); // redraw to remove highlight
        } break;
        case KB_HOME: {
          uint8_t old_step = active_step;
          active_step = 0;
          color_step(old_step, VOICE_WINDOW_FG);
          // refresh_step(active_voice_index, old_step); // redraw to remove highlight
        } break;
        case KB_END: {
          uint8_t old_step = active_step;
          active_step = (STEPS_PER_PATTERN - 1);
          color_step(old_step, VOICE_WINDOW_FG);
          // refresh_step(active_voice_index, old_step); // redraw to remove highlight
        } break;

        case KB_KEY_TAB: {
          active_cell++;
          if(active_cell > 3) active_cell = 0;
        } break;

        case KB_RIGHT_ARROW: {
          update_cell(active_voice, 1);
          last_step_edit = &track.pattern->voices[active_voice_index]->steps[active_step];
          // refresh_step(active_voice_index, active_step);
        } break;
        case KB_LEFT_ARROW: {
          update_cell(active_voice, -1);
          last_step_edit = &track.pattern->voices[active_voice_index]->steps[active_step];
          // refresh_step(active_voice_index, active_step);
        } break;
        case KB_PG_UP: {
          update_cell(active_voice, 2);
          last_step_edit = &track.pattern->voices[active_voice_index]->steps[active_step];
          // refresh_step(active_voice_index, active_step);
        } break;
        case KB_PG_DOWN: {
          update_cell(active_voice, -2);
          last_step_edit = &track.pattern->voices[active_voice_index]->steps[active_step];
          // refresh_step(active_voice_index, active_step);
        } break;
        case KB_INSERT: {
          // copy last_step_edit
          track.pattern->voices[active_voice_index]->steps[active_step].note = last_step_edit->note;
          track.pattern->voices[active_voice_index]->steps[active_step].waveform = last_step_edit->waveform;
          track.pattern->voices[active_voice_index]->steps[active_step].fx1 = last_step_edit->fx1;
          track.pattern->voices[active_voice_index]->steps[active_step].fx2 = last_step_edit->fx2;
          // refresh_step(active_voice_index, active_step);
        } break;
        case KB_DELETE: {
          // delete current step
          track.pattern->voices[active_voice_index]->steps[active_step].note = NOTE_OUT_OF_RANGE;
          track.pattern->voices[active_voice_index]->steps[active_step].waveform = WAVEFORM_OUT_OF_RANGE;
          track.pattern->voices[active_voice_index]->steps[active_step].fx1 = FX_OUT_OF_RANGE;
          track.pattern->voices[active_voice_index]->steps[active_step].fx2 = FX_OUT_OF_RANGE;
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

  if(argc == 1) {
    err = file_load(argv[0]);
    if (err != ERR_SUCCESS) {
      printf("Failed to load data file: %d\n", err);
      exit(err);
    }
  } else {
    pattern_init(track.pattern);
  }

  cursor(0);
  window(&winMain);
  // window(&winDebug);

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


  for(i = 0; i < NUM_VOICES; i++) {
    window_t *w = track.windows[i];
    window(w);
    refresh_track(i);
  }

  // window_puts(&winDebug, track.title);

  state = T_NONE;

  zvb_sound_reset();
  zvb_sound_set_volume(VOL_75);
  zvb_sound_set_hold(VOICEALL, 0);
  zvb_sound_set_voices(VOICEALL, 0, WAV_SQUARE);

  MAP_TEXT();

  ACTIVATE_VOICE(0);
  last_step_edit = &track.pattern->voices[0]->steps[active_step];
  cursor(1);

  while(1) {
    uint8_t action = input();
    switch(action) {
      case ACTION_QUIT:
        goto __exit;
    }

    gfx_wait_vblank(NULL);
    TSTATE_LOG(1);
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

        // // DEBUG
        // gotoxy(win_Pattern1.x + 2, win_Pattern1.y - 2);
        // sprintf(textbuff, "%3u %3u %3u %03u", frames, play_next_step, play_last_step, track.pattern->fx_counter);
        // textcolor(TEXT_COLOR_RED);
        // bgcolor(winMain.bg);
        // cputs(textbuff);
      } break;
      case T_NONE: {
        uint8_t cx = track.windows[active_voice_index]->x + 2;
        uint8_t cy = track.windows[active_voice_index]->y + 2 + active_step;
        switch(active_cell) {
          // case 0: break; // nothing to do here
          case 1: cx += 4; break;
          case 2: cx += 6; break;
          case 3: cx += 9; break;
        }

        gotoxy(cx, cy);
        refresh_step(active_voice_index, active_step);
      } break;
    }
    TSTATE_LOG(1);
    gfx_wait_end_vblank(NULL);
  }

__exit:
  MAP_SOUND();
  zvb_sound_set_voices(VOICEALL, 0, WAV_SQUARE);
  zvb_sound_set_hold(VOICEALL, 1);
  zvb_sound_set_volume(VOL_0);


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

  MAP_TEXT();
  uint16_t size = 0;
  do {
    printf("Enter filename to save recording, press enter to quit without saving %d:\n", size);
    size = sizeof(textbuff);
    err = read(DEV_STDIN, textbuff, &size);
    if(err != ERR_SUCCESS) {
        printf("keyboard error: %d (%02x)\n", err, err);
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