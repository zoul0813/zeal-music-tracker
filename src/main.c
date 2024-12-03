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

#define VOICEALL              (VOICE0 | VOICE1 | VOICE2 | VOICE3)
#define FRAMES_PER_QUARTER    (32U)
#define FRAMES_PER_EIGTH      (FRAMES_PER_QUARTER >> 1)
#define FRAMES_PER_SIXTEENTH  (FRAMES_PER_QUARTER >> 2)
#define FRAMES_PER_STEP       (FRAMES_PER_SIXTEENTH)

#define ACTION_QUIT           1

static const char dummy[4];
static char textbuff[SCREEN_COL80_WIDTH];

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

window_t win_Pattern1 = {
  .x = 2,
  .y = 18,
  .h = 1 + STEPS_PER_PATTERN + 2,
  .w = 17,
  .fg = TEXT_COLOR_LIGHT_GRAY,
  .bg = TEXT_COLOR_DARK_BLUE,
  .flags = WIN_BORDER,
  .title = "Voice 1"
};

window_t win_Pattern2 = {
  .x = 21,
  .y = 18,
  .h = 1 + STEPS_PER_PATTERN + 2,
  .w = 17,
  .fg = TEXT_COLOR_LIGHT_GRAY,
  .bg = TEXT_COLOR_DARK_BLUE,
  .flags = WIN_BORDER,
  .title = "Voice 2"
};

window_t win_Pattern3 = {
  .x = 40,
  .y = 18,
  .h = 1 + STEPS_PER_PATTERN + 2,
  .w = 17,
  .fg = TEXT_COLOR_LIGHT_GRAY,
  .bg = TEXT_COLOR_DARK_BLUE,
  .flags = WIN_BORDER,
  .title = "Voice 3"
};

window_t win_Pattern4 = {
  .x = 59,
  .y = 18,
  .h = 1 + STEPS_PER_PATTERN + 2,
  .w = 17,
  .fg = TEXT_COLOR_LIGHT_GRAY,
  .bg = TEXT_COLOR_DARK_BLUE,
  .flags = WIN_BORDER,
  .title = "Voice 4"
};

window_t winDebug = {
  .x = 2,
  .y = 2,
  .h = 5,
  .w = 25,
  .fg = TEXT_COLOR_LIGHT_GRAY,
  .bg = TEXT_COLOR_BLACK,
  .title = "Debug"
};

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
uint8_t playing_step = STEPS_PER_PATTERN;
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
      step->freq += amount;
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
      step->fx1 += amount;
      break;
    case Cell_Effect2:
      step->fx2 += amount;
      break;
  }
}

void refresh_step(uint8_t voice_index, uint8_t step_index) {
  window_t *w = track.windows[voice_index];
  voice_t *voice = track.pattern->voices[voice_index];
  step_t *step = &voice->steps[step_index];
  const char text[17];
  window_gotoxy(w, 1, step_index + 1);

  // TODO: can this be done once outside of the function?
  uint8_t clr = w->fg;
  if(step_index == active_step) {
    clr = TEXT_COLOR_YELLOW;
  }
  if(state == T_PLAY && step_index == playing_step) {
    clr = TEXT_COLOR_CYAN;
  }

  if(step->freq == 0xFFFF) {
    window_puts_color(w, "-----", clr);
  } else {
    sprintf(text, "%05u", step->freq);
    window_puts_color(w, text, clr);
  }

  if(step->waveform == 0xFF) {
    window_puts_color(w, " - ", clr);
  } else {
    sprintf(text, " %01X ", step->waveform & 0xF);
    window_puts_color(w, text, clr);
  }

  if(step->fx1 == 0xFF) {
    window_puts_color(w, "-- ", clr);
  } else {
    sprintf(text, "%02X ", step->fx1);
    window_puts_color(w, text, clr);
  }

  if(step->fx2 == 0xFF) {
    window_puts_color(w, "-- ", clr);
  } else {
    sprintf(text, "%02X ", step->fx2);
    window_puts_color(w, text, clr);
  }
}

void refresh_steps(uint8_t step_index) {
  if(step_index >= STEPS_PER_PATTERN) return;
  refresh_step(0, step_index);
  refresh_step(1, step_index);
  refresh_step(2, step_index);
  refresh_step(3, step_index);
}

static inline void play_step(step_t *step, sound_voice_t voice) {
  // TODO: manage adjusting things on the fly ... ?
  if(step->freq != 0xFFFF) {
    uint8_t waveform = step->waveform;
    if(waveform > 0x03) waveform = WAV_SQUARE;
    waveform &= 03;
    zvb_sound_set_voices(voice, SOUND_FREQ_TO_DIV(step->freq), waveform);
  }
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

void refresh_track(uint8_t voice_index) {
  // track_t *track = &tracks[track_index];
  voice_t *voice = track.pattern->voices[voice_index];
  window_t *window = track.windows[voice_index];
  uint8_t i;
  window_gotoxy(window, 0, 0);
  window_puts_color(window, " Freq. W F1 F2\n", TEXT_COLOR_WHITE);
  for(i = 0; i < 16; i++) {
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
          frames = 0;
          playing_step = STEPS_PER_PATTERN;
          gotoxy(win_Pattern1.x, win_Pattern1.y - 2);
          textcolor(TEXT_COLOR_RED);
          bgcolor(winMain.bg);
          cputc(242);
        } else {
          state = T_NONE;
          // stop sound
          MAP_SOUND();
          zvb_sound_set_voices(VOICEALL, 0, WAV_SQUARE);
          MAP_TEXT();
          gotoxy(win_Pattern1.x, win_Pattern1.y - 2);
          textcolor(winMain.fg);
          bgcolor(winMain.bg);
          cputs("      "); // clear the "> nnn" from the frame counter
          refresh_steps(playing_step);
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
          if(active_step > 15) active_step = 0;
          refresh_step(active_voice_index, old_step); // redraw to remove highlight
        } break;
        case KB_UP_ARROW: {
          uint8_t old_step = active_step;
          if(active_step > 0) active_step--;
          else active_step = 15;
          refresh_step(active_voice_index, old_step); // redraw to remove highlight
        } break;
        case KB_HOME: {
          uint8_t old_step = active_step;
          active_step = 0;
          refresh_step(active_voice_index, old_step); // redraw to remove highlight
        } break;
        case KB_END: {
          uint8_t old_step = active_step;
          active_step = 15;
          refresh_step(active_voice_index, old_step); // redraw to remove highlight
        } break;

        case KB_KEY_TAB: {
          active_cell++;
          if(active_cell > 3) active_cell = 0;
        } break;

        case KB_RIGHT_ARROW: {
          update_cell(active_voice, 1);
          last_step_edit = &track.pattern->voices[active_voice_index]->steps[active_step];
        } break;
        case KB_LEFT_ARROW: {
          update_cell(active_voice, -1);
          last_step_edit = &track.pattern->voices[active_voice_index]->steps[active_step];
        } break;
        case KB_PG_UP: {
          update_cell(active_voice, 100);
          last_step_edit = &track.pattern->voices[active_voice_index]->steps[active_step];
        } break;
        case KB_PG_DOWN: {
          update_cell(active_voice, -100);
          last_step_edit = &track.pattern->voices[active_voice_index]->steps[active_step];
        } break;
        case KB_INSERT: {
          // copy last_step_edit
          track.pattern->voices[active_voice_index]->steps[active_step].freq = last_step_edit->freq;
          track.pattern->voices[active_voice_index]->steps[active_step].waveform = last_step_edit->waveform;
          track.pattern->voices[active_voice_index]->steps[active_step].fx1 = last_step_edit->fx1;
          track.pattern->voices[active_voice_index]->steps[active_step].fx2 = last_step_edit->fx2;
        } break;
        case KB_DELETE: {
          // delete current step
          track.pattern->voices[active_voice_index]->steps[active_step].freq = 0xFFFF;
          track.pattern->voices[active_voice_index]->steps[active_step].waveform = 0xFF;
          track.pattern->voices[active_voice_index]->steps[active_step].fx1 = 0xFF;
          track.pattern->voices[active_voice_index]->steps[active_step].fx2 = 0xFF;
        } break;
      }
    }
  }
  return 0;
}

int main(int argc, char** argv) {
  zos_err_t err;

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
  window(&winDebug);

  window(&winHelp);
  refresh_help();

  uint8_t i = 0;
  for(i = 0; i < NUM_VOICES; i++) {
    window_t *w = track.windows[i];
    window(w);
    refresh_track(i);
  }

  window_puts(&winDebug, track.title);

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
    switch(state) {
      case T_PLAY: {
        if(frames > FRAMES_PER_QUARTER) frames = 0;
        // if(frames % FRAMES_PER_EIGTH == 0) { }
        if(frames % FRAMES_PER_SIXTEENTH == 0) {
          uint8_t current_step = playing_step;
          playing_step++;
          if(playing_step > 15) playing_step = 0;

          refresh_steps(current_step); // reset previous step
          refresh_steps(playing_step); // update current step
          play_steps(playing_step);
        }
        frames++;
      } break;
      case T_NONE: {
        uint8_t cx = track.windows[active_voice_index]->x + 2;
        uint8_t cy = track.windows[active_voice_index]->y + 2 + active_step;
        switch(active_cell) {
          // case 0: break; // nothing to do here
          case 1: cx += 6; break;
          case 2: cx += 8; break;
          case 3: cx += 11; break;
        }

        gotoxy(cx, cy);
        refresh_step(active_voice_index, active_step);
      } break;
    }

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