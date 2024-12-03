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

#endif