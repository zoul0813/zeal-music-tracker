#include <stddef.h>
#include <zos_vfs.h>
// #include <zgdk.h>

#ifndef TRACKER_H
#define TRACKER_H
/**
 * Cells represent the fields of a Step, each step has 4 cells (Frequency, Waveform, Effect 1, Effect 2)
 *
 * Steps represent the individual playable steps (notes), and the steps modifiers/effects
 *
 * Patterns consist of an array of Steps, and a title.
 *
 * Tracks contain an array of Patterns, a Window reference, Voice/Channel, and a Title
 */

/** ZMT File Format
 *
 * 4-byte header - ZMT{x} - {x} is the version of ZMT
 *
 * 12-byte title
 *
 * 1-byte number of patterns
 *
 *
 */

#define STEPS_PER_PATTERN     32U
#define NUM_VOICES            4U
#define TRACKER_TITLE_LEN     12U

#define CH_PLAY   242U

typedef enum {
  Cell_Frequency = 0,
  Cell_Waveform = 1,
  Cell_Effect1 = 2,
  Cell_Effect2 = 3,
} track_cell_t;

typedef enum {
  FX_NOTE_OFF    = 0x00,
  FX_NOTE_ON     = 0x01,

  // count down from N, reset when roll over
  FX_COUNT_0     = 0xC0,
  FX_COUNT_1     = 0xC1,
  FX_COUNT_2     = 0xC2,
  FX_COUNT_3     = 0xC3,
  FX_COUNT_4     = 0xC4,
  FX_COUNT_5     = 0xC5,
  FX_COUNT_6     = 0xC6,
  FX_COUNT_7     = 0xC7,
  FX_COUNT_8     = 0xC8,

  // goto step N
  FX_GOTO_0      = 0xD0,
  FX_GOTO_1      = 0xD1,
  FX_GOTO_2      = 0xD2,
  FX_GOTO_3      = 0xD3,
  FX_GOTO_4      = 0xD4,
  FX_GOTO_5      = 0xD5,
  FX_GOTO_6      = 0xD6,
  FX_GOTO_7      = 0xD7,
  FX_GOTO_8      = 0xD8,
  FX_GOTO_9      = 0xD9,
  FX_GOTO_10     = 0xDA,
  FX_GOTO_11     = 0xDB,
  FX_GOTO_12     = 0xDC,
  FX_GOTO_13     = 0xDD,
  FX_GOTO_14     = 0xDE,
  FX_GOTO_15     = 0xDF,
  FX_GOTO_16     = 0xE0,
  FX_GOTO_17     = 0xE1,
  FX_GOTO_18     = 0xE2,
  FX_GOTO_19     = 0xE3,
  FX_GOTO_20     = 0xE4,
  FX_GOTO_21     = 0xE5,
  FX_GOTO_22     = 0xE6,
  FX_GOTO_23     = 0xE7,
  FX_GOTO_24     = 0xE8,
  FX_GOTO_25     = 0xE9,
  FX_GOTO_26     = 0xEA,
  FX_GOTO_27     = 0xEB,
  FX_GOTO_28     = 0xEC,
  FX_GOTO_29     = 0xED,
  FX_GOTO_30     = 0xEE,
  FX_GOTO_31     = 0xEF,

  // Volume, 8-steps (per channel???)
  FX_VOL_00_0    = 0xF0,
  FX_VOL_12_5    = 0xF1,
  FX_VOL_25_0    = 0xF2,
  FX_VOL_37_5    = 0xF3,
  FX_VOL_50_0    = 0xF4,
  FX_VOL_62_5    = 0xF5,
  FX_VOL_75_0    = 0xF6,
  FX_VOL_87_5    = 0xF7,
  FX_VOL_100     = 0xF8,
} FX;
typedef uint8_t fx_t;
typedef uint8_t fx_attr_t;
#define FX_OUT_OF_RANGE         (0xFF)

#define NUM_NOTES               (12 * 9 + 1)
typedef uint16_t note_t;
typedef char note_name_t[3];
extern note_t NOTES[NUM_NOTES]; // 108 notes
extern note_name_t NOTE_NAMES[NUM_NOTES]; // 108 notes
#define NOTE_OUT_OF_RANGE       (0x80)
#define NOTE_OFF                (NUM_NOTES - 1)
#define NOTE_MIDDLE_C           (12 * 4)

// this is derived from sound_waveform_t
typedef uint8_t waveform_t;
#define WAVEFORM_OUT_OF_RANGE   (0xFF)

typedef struct {
  // uint16_t freq;
  note_t note;
  waveform_t waveform;
  fx_t fx1;
  fx_t fx2;

  // values for fx
  fx_attr_t fx1_attr;
  fx_attr_t fx2_attr;
} step_t;

#define FX_ATTRS    2

typedef struct {
  uint8_t voice;
  step_t steps[STEPS_PER_PATTERN];
} voice_t;

typedef struct {
  char title[TRACKER_TITLE_LEN];
  voice_t* voices[NUM_VOICES];
  // TODO: add sample voice?
} pattern_t;


zos_err_t pattern_load(pattern_t *pattern, zos_dev_t dev);
zos_err_t pattern_save(pattern_t *pattern, zos_dev_t dev);
void pattern_init(pattern_t *pattern);
#endif