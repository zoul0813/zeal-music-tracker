#include <stdio.h>
#include <zos_sys.h>
#include <zos_vfs.h>
#include <zos_errors.h>
#include <zos_keyboard.h>
#include <zos_video.h>
#include <zgdk.h>
#include <conio.h>
#include <windows.h>

#define FRAMES_PER_QUARTER    (32U)
#define FRAMES_PER_EIGTH      (FRAMES_PER_QUARTER >> 1)
#define FRAMES_PER_SIXTEENTH  (FRAMES_PER_QUARTER >> 2)
#define FRAMES_PER_STEP       (FRAMES_PER_SIXTEENTH)
#define STEPS_PER_PATTERN     16U

/**
 * Cells represent the fields of a Step, each step has 4 cells (Frequency, Waveform, Effect 1, Effect 2)
 *
 * Steps represent the individual playable steps (notes), and the steps modifiers/effects
 *
 * Patterns consist of an array of Steps, and a title.
 *
 * Tracks contain an array of Patterns, a Window reference, Voice/Channel, and a Title
 */

typedef enum {
  Cell_Frequency = 0,
  Cell_Waveform = 1,
  Cell_Effect1 = 2,
  Cell_Effect2 = 3,
} Cell;

typedef struct {
  uint16_t freq;
  uint8_t waveform;
  uint8_t f1;
  uint8_t f2;
} step_t;

typedef struct {
  char title[12];
  step_t steps[STEPS_PER_PATTERN];
} pattern_t;

typedef struct {
  char title[12];
  uint8_t voice;
  window_t *window;
  pattern_t* pattern;
} track_t;

pattern_t pattern1 = { .title = "Pattern 1", };
pattern_t pattern2 = { .title = "Pattern 2", };
pattern_t pattern3 = { .title = "Pattern 3", };
pattern_t pattern4 = { .title = "Pattern 4", };

// pattern_t* patterns[4] = {
//   &pattern1,
//   &pattern2,
//   &pattern3,
//   &pattern4,
// };

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

window_t winTrack1 = {
  .x = 2,
  .y = 18,
  .h = 1 + STEPS_PER_PATTERN + 2,
  .w = 17,
  .fg = TEXT_COLOR_LIGHT_GRAY,
  .bg = TEXT_COLOR_DARK_BLUE,
  .flags = WIN_BORDER,
  .title = "Track 1"
};

window_t winTrack2 = {
  .x = 21,
  .y = 18,
  .h = 1 + STEPS_PER_PATTERN + 2,
  .w = 17,
  .fg = TEXT_COLOR_LIGHT_GRAY,
  .bg = TEXT_COLOR_DARK_BLUE,
  .flags = WIN_BORDER,
  .title = "Track 2"
};

window_t winTrack3 = {
  .x = 40,
  .y = 18,
  .h = 1 + STEPS_PER_PATTERN + 2,
  .w = 17,
  .fg = TEXT_COLOR_LIGHT_GRAY,
  .bg = TEXT_COLOR_DARK_BLUE,
  .flags = WIN_BORDER,
  .title = "Track 3"
};

window_t winTrack4 = {
  .x = 59,
  .y = 18,
  .h = 1 + STEPS_PER_PATTERN + 2,
  .w = 17,
  .fg = TEXT_COLOR_LIGHT_GRAY,
  .bg = TEXT_COLOR_DARK_BLUE,
  .flags = WIN_BORDER,
  .title = "Track 4"
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

track_t tracks[] = {
  { .title = "Track 1", .voice = 0, .window = &winTrack1 },
  { .title = "Track 2", .voice = 1, .window = &winTrack2 },
  { .title = "Track 3", .voice = 2, .window = &winTrack3 },
  { .title = "Track 4", .voice = 3, .window = &winTrack4 },
};

track_t* active_track = NULL;
uint8_t active_track_index = 0;
uint8_t active_cell = 0;
uint8_t active_step = 0;

const uint8_t NUM_TRACKS = DIM(tracks);

#define ACTIVATE_TRACK(index) \
  active_track_index = index; \
  active_track = &tracks[index]; \
  window_gotoxy(active_track->window, 0, 0); \
  gotoxy(active_track->window->x + 2, active_track->window->y + 2);

static void init_pattern(pattern_t *pattern) {
  for(uint8_t i = 0; i < STEPS_PER_PATTERN; i++) {
    pattern->steps[i].freq = 0;
    pattern->steps[i].waveform = WAV_SQUARE;
    pattern->steps[i].f1 = 0x00;
    pattern->steps[i].f2 = 0x00;
  }
}

static void update_cell(track_t *track, int8_t amount) {
  switch(active_cell) {
    case Cell_Frequency:
      track->pattern->steps[active_step].freq += amount;
      break;
    case Cell_Waveform:
      track->pattern->steps[active_step].waveform += amount;
      break;
    case Cell_Effect1:
      track->pattern->steps[active_step].f1 += amount;
      break;
    case Cell_Effect2:
      track->pattern->steps[active_step].f2 += amount;
      break;
  }
}

static void refresh_step(track_t *track, uint8_t step_index) {
  // track_t *track = &tracks[track_index];
  pattern_t *pattern = track->pattern;
  step_t *step = &pattern->steps[step_index];
  char text[13];
  window_gotoxy(track->window, 0, step_index + 1);
  if(step == NULL) {
    window_puts(track->window, " ----- - -- --");
  } else {
    sprintf(
      text,
      " %05u %01X %02X %02X",
      step->freq,
      step->waveform & 0x0F,
      step->f1,
      step->f2
    );
    window_puts(track->window, text);
  }
}

static void refresh_track(uint8_t track_index) {
  track_t *track = &tracks[track_index];
  uint8_t i;
  window_gotoxy(track->window, 0,0);
  window_puts_color(track->window, " Freq. V F1 F2\n", TEXT_COLOR_WHITE);
  for(i = 0; i < 16; i++) {
    refresh_step(track, i);
  }
}

int main(void) {
  zos_err_t err;

  cursor(0);
  window(&winMain);
  // window(&winTrack1);
  // window(&winTrack2);
  // window(&winTrack3);
  // window(&winTrack4);

  window(&winDebug);

  // window_puts(&winMain, "Zeal Music Tracker");


  uint8_t i = 0;
  for(i = 0; i < NUM_TRACKS; i++) {
    track_t *track = &tracks[i];
    window(track->window);
    refresh_track(i);
  }

  ACTIVATE_TRACK(0);
  cursor(1);

  char text[78];
  while(1) {
    unsigned char c = cgetc();
    window_gotoxy(&winDebug, 0, 0);
    sprintf(text, "Character: %d (%02x)\n", c, c);
    window_puts(&winDebug, text);

    switch(c) {
      case KB_ESC:
        goto exit;
      case KB_KEY_1:
        ACTIVATE_TRACK(0);
        break;
      case KB_KEY_2:
        ACTIVATE_TRACK(1);
        break;
      case KB_KEY_3:
        ACTIVATE_TRACK(2);
        break;
      case KB_KEY_4:
        ACTIVATE_TRACK(3);
        break;

      case KB_DOWN_ARROW:
        active_step++;
        if(active_step > 15) active_step = 0;
        sprintf(text, "Step: %d (%02x)\n", active_step, active_step);
        window_puts(&winDebug, text);
        break;
      case KB_UP_ARROW:
        if(active_step > 1) active_step--;
        else active_step = 15;
        sprintf(text, "Step: %d (%02x)\n", active_step, active_step);
        window_puts(&winDebug, text);
        break;

      case KB_KEY_TAB:
        active_cell++;
        if(active_cell > 3) active_cell = 0;
        sprintf(text, "Cell: %d (%02x)\n", active_cell, active_cell);
        window_puts(&winDebug, text);
        break;

      case KB_RIGHT_ARROW:
        update_cell(active_track, 1);
        refresh_step(active_track, active_step);
        break;
      case KB_LEFT_ARROW:
        update_cell(active_track, -1);
        refresh_step(active_track, active_step);
        break;
    }

    uint8_t cx = active_track->window->x + 2;
    uint8_t cy = active_track->window->y + 2 + active_step;
    switch(active_cell) {
      // case 0: break;
      case 1: cx += 6; break;
      case 2: cx += 8; break;
      case 3: cx += 11; break;
    }

    gotoxy(cx, cy); // active_track->window->x + 2 + x, active_track->window->y + 1 + y);
  }

exit:

  // reset the screen
  err = ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);

  printf("Tracks %d", NUM_TRACKS);

  return err;
}