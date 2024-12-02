#include <stdio.h>
#include <string.h>
#include <zos_sys.h>
#include <zos_vfs.h>
#include <zos_errors.h>
#include <zos_keyboard.h>
#include <zos_video.h>
#include <zvb_sound.h>
// #include <zgdk.h>
#include "conio.h"
#include "windows.h"
#include "tracker.h"

#define FRAMES_PER_QUARTER    (32U)
#define FRAMES_PER_EIGTH      (FRAMES_PER_QUARTER >> 1)
#define FRAMES_PER_SIXTEENTH  (FRAMES_PER_QUARTER >> 2)
#define FRAMES_PER_STEP       (FRAMES_PER_SIXTEENTH)

#define ACTION_QUIT           1

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
  .title = "Voice 1",
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
uint8_t playing_step = 0;
uint16_t frames = 0;
char text[78];

music_state_t state = T_NONE;

#define ACTIVATE_VOICE(index) \
  active_voice_index = index; \
  active_voice = track.pattern->voices[index]; \
  window_gotoxy(track.windows[index], 0, 0); \
  gotoxy(track.windows[index]->x + 2, track.windows[index]->y + 2);

static void init_pattern(pattern_t *pattern) {
  for(uint8_t i = 0; i < NUM_VOICES; i++) {
    for(uint8_t j = 0; j < STEPS_PER_PATTERN; j++) {
      pattern->voices[i]->steps[j].freq = 0xFFFF;
      pattern->voices[i]->steps[j].waveform = 0xFF;
      pattern->voices[i]->steps[j].fx1 = 0xFF;
      pattern->voices[i]->steps[j].fx2 = 0xFF;
    }
  }
}

static void update_cell(voice_t *voice, int8_t amount) {
  step_t *step = &voice->steps[active_step];
  switch(active_cell) {
    case Cell_Frequency:
      step->freq += amount;
      break;
    case Cell_Waveform:
      if(step->waveform == 0xFF) {
        step->waveform = 0x00;
      }
      step->waveform += amount;
      if((step->waveform < 0xFF) && (step->waveform > 0x0F)) {
        step->waveform = 0x00;
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

static void refresh_step(uint8_t voice_index, uint8_t step_index) {
  window_t *w = track.windows[voice_index];
  voice_t *voice = track.pattern->voices[voice_index];
  step_t *step = &voice->steps[step_index];
  const char text[17];
  window_gotoxy(w, 1, step_index + 1);

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

static void play_step(uint8_t step_index) {
  step_t *step1 = &track.pattern->voices[0]->steps[step_index];
  step_t *step2 = &track.pattern->voices[1]->steps[step_index];
  step_t *step3 = &track.pattern->voices[2]->steps[step_index];
  step_t *step4 = &track.pattern->voices[3]->steps[step_index];
  if(step1->freq != 0xFF) {
    zvb_sound_set_voices(VOICE0, step1->freq, step1->waveform);
  }
  if(step2->freq != 0xFF) {
    zvb_sound_set_voices(VOICE0, step2->freq, step2->waveform);
  }
  if(step3->freq != 0xFF) {
    zvb_sound_set_voices(VOICE0, step3->freq, step3->waveform);
  }
  if(step3->freq != 0xFF) {
    zvb_sound_set_voices(VOICE0, step3->freq, step4->waveform);
  }
}

static void refresh_track(uint8_t voice_index) {
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

static void refresh_help(void) {
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

zos_err_t file_load(void) {
  zos_dev_t file_dev = open("track.zmt", O_RDONLY);
  if(file_dev < 0) {
    printf("failed to open file, %d (%02x)", -file_dev, -file_dev);
    init_pattern(track.pattern);
    return -file_dev;
  } else {
    uint16_t size = 0;
    char text[TRACKER_TITLE_LEN] = { 0x20 };

    size = 3;
    read(file_dev, text, &size); // format header
    // printf("Format: %03s\n", text);

    size = sizeof(uint8_t);
    read(file_dev, text, &size); // version header
    // printf("Version: %d\n", text[0]);

    size = TRACKER_TITLE_LEN;
    read(file_dev, text, &size); // track title
    memcpy(track.title, text, size);
    track.title[TRACKER_TITLE_LEN] = 0x00; // NUL
    // printf("Track: %12s (read: %d)\n", track.title, size);

    size = sizeof(uint8_t);
    read(file_dev, text, &size); // pattern count
    // printf("Patterns: %d\n", text[0]);

    pattern_load(track.pattern, file_dev);

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

zos_err_t file_save(void) {
  zos_dev_t file_dev = open("track.zmt", O_WRONLY | O_CREAT | O_TRUNC);
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

static uint8_t key_buffer[36];
uint8_t input(void) {
  // unsigned char c = cgetc();

  uint16_t size = sizeof(key_buffer);
  zos_err_t err = read(DEV_STDIN, key_buffer, &size);
  if (err != ERR_SUCCESS) {
    printf("Failed to read from DEV_STDIN\n");
    exit(3);
  }
  // window_gotoxy(&winDebug, 0, 0);
  // sprintf(text, "Character: %d (%02x)\n", c, c);
  // window_puts(&winDebug, text);

  uint8_t i = 0;
  // uint8_t released = 0;
  for(i = 0; i < size; i++) {
    const char c = key_buffer[i];

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

      /** TRANSPORT */
      case KB_KEY_SPACE: {
        if(state == T_NONE) {
          state = T_PLAY;
          frames = 0;
          gotoxy(win_Pattern1.x, win_Pattern1.y - 2);
          textcolor(TEXT_COLOR_RED);
          bgcolor(winMain.bg);
          cputc(242);
        } else {
          state = T_NONE;
          gotoxy(win_Pattern1.x, win_Pattern1.y - 2);
          textcolor(winMain.fg);
          bgcolor(winMain.bg);
          cputs("      "); // clear the "> nnn" from the frame counter

        }
        // music_transport(state, music_frame());
      } break;
    }
  }
  return 0;
}

int main(void) {
  zos_err_t err;

  err = keyboard_init();
  if (err != ERR_SUCCESS) {
    printf("Failed to init keyboard: %d\n", err);
    exit(1);
  }
  err = keyboard_flush();
  if (err != ERR_SUCCESS) {
    printf("Failed to flush keyboard: %d\n", err);
    exit(1);
  }

  // zvb_sound_initialize(1);
  // zvb_sound_set_voices(VOICE0 | VOICE1 | VOICE2 | VOICE3, 0, WAV_SQUARE);
  // zvb_sound_set_hold(VOICE0 | VOICE1 | VOICE2 | VOICE3, 0);
  // zvb_sound_set_volume(VOL_75);
  // zvb_sound_reset();

  err = file_load();

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

  ACTIVATE_VOICE(0);
  last_step_edit = &track.pattern->voices[0]->steps[active_step];
  cursor(1);

  while(1) {
    gfx_wait_vblank(NULL);
    if(state == T_PLAY) {
      frames++;
      if(frames > FRAMES_PER_QUARTER) frames = 0;
      // if(frames % FRAMES_PER_EIGTH == 0) { }
      if(frames % FRAMES_PER_SIXTEENTH == 0) {
        gotoxy(win_Pattern1.x + 2, win_Pattern1.y - 2);
        sprintf(text, "%03u", frames);
        cputs(text);
        uint8_t current_step = playing_step;
        playing_step++;
        if(playing_step > 15) playing_step = 0;

        // reset previous step
        refresh_step(0, current_step);
        refresh_step(1, current_step);
        refresh_step(2, current_step);
        refresh_step(3, current_step);
        // update current step
        refresh_step(0, playing_step);
        refresh_step(1, playing_step);
        refresh_step(2, playing_step);
        refresh_step(3, playing_step);

        // play_step(playing_step);
      }
    }

    uint8_t action = input();
    switch(action) {
      case ACTION_QUIT:
        goto exit;
    }

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
    gfx_wait_end_vblank(NULL);
  }

exit:

  err = file_save();

  // reset the screen
  err = ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);

  return err;
}