#include <stdint.h>
#ifndef ARRANGE_H
#define ARRANGE_H

void arrange_show(uint8_t index);
uint8_t arrange_keypress_handler(unsigned char key);
void arrange_current_step_handler(uint8_t current_step);
void arrange_current_arrangement_handler(uint8_t current_arragement);

#endif