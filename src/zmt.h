#ifndef ZMT_H
#define ZMT_H

#ifdef EMULATOR
__sfr __at(0x86) debug_register; // t-state counter
#define TSTATE_LOG(counter) debug_register = counter;
#else
#define TSTATE_LOG(counter)
#endif

typedef enum {
  T_NONE = 0,
  T_PLAY = 1,
} playback_state_t;

#define VOICE_WINDOW_FG   TEXT_COLOR_LIGHT_GRAY
#define VOICE_WINDOW_BG   TEXT_COLOR_DARK_BLUE
#define VOICE_WINDOW_HL1  TEXT_COLOR_CYAN
#define VOICE_WINDOW_Hl2  TEXT_COLOR_YELLOW

#define CELL_OFFSET_FREQ   0
#define CELL_OFFSET_WAVE   4
#define CELL_OFFSET_F1     6
#define CELL_OFFSET_F2     9

#define ACTION_QUIT           1
#endif