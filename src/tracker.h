#include <stddef.h>
#include <zos_vfs.h>
#include <zgdk.h>

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

#define STEPS_PER_PATTERN     16U
#define NUM_VOICES            4U
#define TRACKER_TITLE_LEN     12U

typedef enum {
  Cell_Frequency = 0,
  Cell_Waveform = 1,
  Cell_Effect1 = 2,
  Cell_Effect2 = 3,
} track_cell_t;

typedef struct {
  uint16_t freq;
  uint8_t waveform;
  uint8_t fx1;
  uint8_t fx2;

} step_t;

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