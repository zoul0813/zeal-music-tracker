#include <stdio.h>
#include <string.h>
#include <zos_keyboard.h>
#include "tracker.h"
#include "shared.h"
#include "windows.h"
#include "pattern.h"


voice_t* active_voice = NULL;
uint8_t active_voice_index = 0;
uint8_t active_cell = 0;
uint8_t active_step = 0;
step_t *last_step_edit = NULL;
uint8_t previous_step = 0;

window_t win_Indicators = {
  .x = 2,
  .y = PATTERN_WIN_Y,
  .w = 2,
  .h = STEPS_PER_PATTERN,
  .fg = TEXT_COLOR_LIGHT_GRAY,
  .bg = TEXT_COLOR_DARK_GRAY,
  .flags = 0,
};

window_t win_Pattern1 = {
  .x = PATTERN_WIN_X + (PATTERN_WIN_WIDTH * 0),
  .y = PATTERN_WIN_Y,
  .h = 1 + STEPS_PER_PATTERN + 2,
  .w = PATTERN_WIN_WIDTH,
  .fg = PATTERN_WINDOW_FG,
  .bg = PATTERN_WINDOW_BG,
  .flags = WIN_BORDER | WIN_SHADOW,
  .title = "Voice 1"
};

window_t win_Pattern2 = {
  .x = PATTERN_WIN_X + (PATTERN_WIN_WIDTH * 1) + 1 + 1,
  .y = PATTERN_WIN_Y,
  .h = 1 + STEPS_PER_PATTERN + 2,
  .w = PATTERN_WIN_WIDTH,
  .fg = PATTERN_WINDOW_FG,
  .bg = PATTERN_WINDOW_BG,
  .flags = WIN_BORDER | WIN_SHADOW,
  .title = "Voice 2"
};

window_t win_Pattern3 = {
  .x = PATTERN_WIN_X + (PATTERN_WIN_WIDTH * 2) + 2 + 2,
  .y = PATTERN_WIN_Y,
  .h = 1 + STEPS_PER_PATTERN + 2,
  .w = PATTERN_WIN_WIDTH,
  .fg = PATTERN_WINDOW_FG,
  .bg = PATTERN_WINDOW_BG,
  .flags = WIN_BORDER | WIN_SHADOW,
  .title = "Voice 3"
};

window_t win_Pattern4 = {
  .x = PATTERN_WIN_X + (PATTERN_WIN_WIDTH * 3) + 3 + 3,
  .y = PATTERN_WIN_Y,
  .h = 1 + STEPS_PER_PATTERN + 2,
  .w = PATTERN_WIN_WIDTH,
  .fg = PATTERN_WINDOW_FG,
  .bg = PATTERN_WINDOW_BG,
  .flags = WIN_BORDER | WIN_SHADOW,
  .title = "Voice 4"
};

window_t* windows[NUM_VOICES] = {
    &win_Pattern1,
    &win_Pattern2,
    &win_Pattern3,
    &win_Pattern4,
};

void pattern_update_cell(voice_t *voice, int8_t amount) {
  step_t *step = &voice->steps[active_step];
  window_t *w = windows[active_voice_index];
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
      sprintf(textbuff, "%.3s", NOTE_NAMES[step->note]);
      // TODO: do this better? memcpy the textbuff into SCR_TEXT[y][x]?
      window_gotoxy(w, CELL_OFFSET_FREQ, active_step + 1);
      window_puts_color(w, textbuff, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
      break;
    case Cell_Waveform:
      if(amount < 0 && step->waveform == 0xFF) {
        step->waveform = 0x04;
      }
      step->waveform += amount;
      if(step->waveform > 0x03) {
        step->waveform = 0xFF;
      }
      sprintf(textbuff, "%01X", step->waveform & 0x0F);
      window_gotoxy(w, CELL_OFFSET_WAVE, active_step + 1);
      window_puts_color(w, textbuff, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
      break;
    case Cell_Effect1:
      if(amount > 1) amount = 16;
      if(amount < -1) amount = -16;
      step->fx1 += amount;
      sprintf(textbuff, "%02X", step->fx1);
      window_gotoxy(w, CELL_OFFSET_F1, active_step + 1);
      window_puts_color(w, textbuff, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
      break;
    case Cell_Effect2:
      if(amount > 1) amount = 16;
      if(amount < -1) amount = -16;
      step->fx2 += amount;
      sprintf(textbuff, "%02X", step->fx2);
      window_gotoxy(w, CELL_OFFSET_F2, active_step + 1);
      window_puts_color(w, textbuff, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
      break;
  }
}

void pattern_color_step(uint8_t step_index, uint8_t color) {
  uint8_t x1 = windows[0]->x + 2;
  uint8_t x2 = windows[1]->x + 2;
  uint8_t x3 = windows[2]->x + 2;
  uint8_t x4 = windows[3]->x + 2;

  uint8_t y = windows[0]->y + 1 + step_index + 1;
  uint8_t bg = windows[0]->bg;

  uint8_t clr = (bg << 4) | (color & 0x0F);

  text_map_vram();
  // MAGIC: 12 = magic length of the step text :)
  SCR_COLOR[y][2] = COLOR(color, TEXT_COLOR_DARK_GRAY);
  SCR_COLOR[y][3] = COLOR(color, TEXT_COLOR_DARK_GRAY);
  for(uint8_t i = 0; i < 12; i++) {
      SCR_COLOR[y][x1 + i] = clr;
      SCR_COLOR[y][x2 + i] = clr;
      SCR_COLOR[y][x3 + i] = clr;
      SCR_COLOR[y][x4 + i] = clr;
  }
  text_demap_vram();
}

void pattern_color_cell(uint8_t step_index, uint8_t cell_index, uint8_t color) {
  uint8_t y = windows[active_voice_index]->y + 1 + step_index + 1;
  uint8_t bg = windows[active_voice_index]->bg;
  uint8_t offset = 0;
  uint8_t x = windows[active_voice_index]->x + 1;
  uint8_t width = 1;
  uint8_t i = 0;

  switch(cell_index) {
    case 0: offset = CELL_OFFSET_FREQ; break;
    case 1: offset = CELL_OFFSET_WAVE; break;
    case 2: offset = CELL_OFFSET_F1; break;
    case 3: offset = CELL_OFFSET_F2; break;
  }

  x+=offset;

  text_map_vram();
  switch(active_cell) {
    case Cell_Frequency: width = 3; break;
    case Cell_Waveform: break;
    case Cell_Effect1: width = 2; break;
    case Cell_Effect2: width = 2; break;
  }
  for(i = 0; i < width; i++) {
    SCR_COLOR[y][x+i] = color;
  }
  text_demap_vram();
}

void pattern_refresh_step(uint8_t voice_index, uint8_t step_index) {
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

void pattern_refresh_steps(void) {
  uint8_t i = 0, j = 0;
  for(i = 0; i < NUM_VOICES; i++) {
    window_t *w = windows[i];
    window(w);

    voice_t* voice = &active_pattern->voices[i];
    window_puts_color(w, " No. W F1 F2\n", COLOR(TEXT_COLOR_WHITE, PATTERN_WINDOW_BG));
    for(j = 0; j < STEPS_PER_PATTERN; j++) {
      pattern_refresh_step(i, j);
    }
  }

  pattern_color_step(0, PATTERN_WINDOW_HL1);
  pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
}

void pattern_show(uint8_t index) {
  active_pattern_index = index;
  active_pattern = track.patterns[active_pattern_index];

  // pattern step indicators
  window(&win_Indicators);
  window_gotoxy(&win_Indicators, 0, 0);
  sprintf(textbuff, "P%01X", active_pattern_index & 0x0F);
  window_puts(&win_Indicators, textbuff);
  window_gotoxy(&win_Indicators, 0, 2);
  for(uint8_t i = 0; i <= FX_GOTO_31 - FX_GOTO_0; i++) {
    sprintf(textbuff, "%02X", i + FX_GOTO_0);
    window_puts(&win_Indicators, textbuff);
  }

  active_voice_index = active_voice_index;
  active_voice = &active_pattern->voices[active_voice_index];
  pattern_refresh_steps();
}

void pattern_current_step_handler(uint8_t current_step) {
  pattern_color_step(previous_step, PATTERN_WINDOW_FG);
  pattern_color_step(current_step, PATTERN_WINDOW_HL2);
  previous_step = current_step;
}

void pattern_keypress_handler(unsigned char key) {
  switch(key) {
    /** CELL EDIT */
    case KB_LEFT_ARROW: {
      pattern_update_cell(active_voice, -1);
      last_step_edit = &active_voice->steps[active_step];
    } break;
    case KB_RIGHT_ARROW: {
      pattern_update_cell(active_voice, 1);
      last_step_edit = &active_voice->steps[active_step];
    } break;
    case KB_PG_DOWN: {
      pattern_update_cell(active_voice, -2);
      last_step_edit = &active_voice->steps[active_step];
    } break;
    case KB_PG_UP: {
      pattern_update_cell(active_voice, 2);
      last_step_edit = &active_voice->steps[active_step];
    } break;
    case KB_INSERT: {
      memcpy(&active_voice->steps[active_step], last_step_edit, sizeof(step_t));
      pattern_refresh_step(active_voice_index, active_step);
      pattern_color_step(active_step, PATTERN_WINDOW_HL1);
      pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
    } break;
    case KB_DELETE: {
      active_voice->steps[active_step].note = NOTE_OUT_OF_RANGE;
      active_voice->steps[active_step].waveform = WAVEFORM_OUT_OF_RANGE;
      active_voice->steps[active_step].fx2 = FX_OUT_OF_RANGE;
      active_voice->steps[active_step].fx2 = FX_OUT_OF_RANGE;
      pattern_refresh_step(active_voice_index, active_step);
      pattern_color_step(active_step, PATTERN_WINDOW_HL1);
      pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
    } break;

    /** CELL NAVIGATION */
    case KB_KEY_TAB: {
      pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, PATTERN_WINDOW_BG));
      active_cell++;
      if(active_cell > 3) active_cell = 0;
      pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
    } break;

    /** STEP NAVIGATION */
    case KB_UP_ARROW: {
      pattern_color_step(active_step, PATTERN_WINDOW_FG);
      if(active_step > 0) active_step--;
      else active_step = (STEPS_PER_PATTERN - 1);
      pattern_color_step(active_step, PATTERN_WINDOW_HL1);
      pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
    } break;
    case KB_DOWN_ARROW: {
      pattern_color_step(active_step, PATTERN_WINDOW_FG);
      active_step++;
      if(active_step > (STEPS_PER_PATTERN - 1)) active_step = 0;
      pattern_color_step(active_step, PATTERN_WINDOW_HL1);
      pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
    } break;
    case KB_HOME: {
      pattern_color_step(active_step, PATTERN_WINDOW_FG);
      active_step = 0;
      pattern_color_step(active_step, PATTERN_WINDOW_HL1);
      pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
    } break;
    case KB_END: {
      pattern_color_step(active_step, PATTERN_WINDOW_FG);
      active_step = (STEPS_PER_PATTERN - 1);
      pattern_color_step(active_step, PATTERN_WINDOW_HL1);
      pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
    } break;

    /** VOICE NAVIGATION */
    case KB_KEY_1: {
      pattern_color_step(active_step, PATTERN_WINDOW_HL1);
      active_voice_index = 0;
      active_voice = &active_pattern->voices[active_voice_index];
      pattern_color_step(active_step, PATTERN_WINDOW_HL1);
      pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
    } break;
    case KB_KEY_2: {
      pattern_color_step(active_step, PATTERN_WINDOW_HL1);
      active_voice_index = 1;
      active_voice = &active_pattern->voices[active_voice_index];
      pattern_color_step(active_step, PATTERN_WINDOW_HL1);
      pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
    } break;
    case KB_KEY_3: {
      pattern_color_step(active_step, PATTERN_WINDOW_HL1);
      active_voice_index = 2;
      active_voice = &active_pattern->voices[active_voice_index];
      pattern_color_step(active_step, PATTERN_WINDOW_HL1);
      pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
    } break;
    case KB_KEY_4: {
      pattern_color_step(active_step, PATTERN_WINDOW_HL1);
      active_voice_index = 3;
      active_voice = &active_pattern->voices[active_voice_index];
      pattern_color_step(active_step, PATTERN_WINDOW_HL1);
      pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
    } break;

    /** PATTERN NAVIGATION */
    case KB_KEY_LEFT_BRACKET: {
      // previous pattern
      active_pattern_index = zmt_pattern_prev(&track);
      active_pattern = track.patterns[active_pattern_index];
      active_voice = &active_pattern->voices[active_voice_index];
      pattern_refresh_steps();

      window_gotoxy(&win_Indicators, 0, 0);
      sprintf(textbuff, "P%01X", active_pattern_index & 0x0F);
      window_puts(&win_Indicators, textbuff);
    } break;
    case KB_KEY_RIGHT_BRACKET: {
      // next pattern
      active_pattern_index = zmt_pattern_next(&track);
      active_pattern = track.patterns[active_pattern_index];
      active_voice = &active_pattern->voices[active_voice_index];
      pattern_refresh_steps();

      window_gotoxy(&win_Indicators, 0, 0);
      sprintf(textbuff, "P%01X", active_pattern_index & 0x0F);
      window_puts(&win_Indicators, textbuff);
    } break;
    case KB_KEY_N: {
      // new pattern
      if(track.pattern_count < NUM_PATTERNS) {
        track.pattern_count++;
        active_pattern_index = track.pattern_count - 1;
      }
      active_pattern_index = zmt_pattern_set(&track, active_pattern_index);
      active_pattern = track.patterns[active_pattern_index];
      active_voice = &active_pattern->voices[active_voice_index];
      zmt_pattern_init(active_pattern);

      window_gotoxy(&win_Indicators, 0, 0);
      sprintf(textbuff, "P%01X", active_pattern_index & 0x0F);
      window_puts(&win_Indicators, textbuff);

      pattern_refresh_steps();
      pattern_color_step(active_step, PATTERN_WINDOW_HL1);
      pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
    } break;
  }
}