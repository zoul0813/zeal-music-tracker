#include "shared.h"

#ifndef CONFIRM_DIALOG_H
#define CONFIRM_DIALOG_H

void confirm_dialog_show(View view, const char* message);
uint8_t confirm_keypress_handler(unsigned char key);
#endif