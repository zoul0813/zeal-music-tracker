#include <stdint.h>

#ifndef PATTERN_H
#define PATTERN_H

#define PATTERN_WIN_X     5U
#define PATTERN_WIN_Y     3U
#define PATTERN_WIN_WIDTH 15U

#define CELL_OFFSET_FREQ   1
#define CELL_OFFSET_WAVE   5
#define CELL_OFFSET_F1     7
#define CELL_OFFSET_F2     10

void pattern_show(uint8_t index);
void pattern_keypress_handler(unsigned char key);
void pattern_current_step_handler(uint8_t current_step);

#endif