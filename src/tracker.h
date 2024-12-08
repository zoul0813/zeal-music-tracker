#include <stddef.h>
#include <zos_vfs.h>
#include <zvb_hardware.h>
// #include <zgdk.h>

#ifndef TRACKER_H
#define TRACKER_H
/**
 * Cells represent the fields of a Step, each step has 4 cells (Frequency, Waveform, Effect 1, Effect 2)
 * In addition to 4 cells, each step also has 2 fx attributes (for remembering fx values)
 *
 * Steps represent the individual playable steps (notes), and the steps modifiers/fx
 *
 * Patterns consist of an array of Voices, with Steps.
 *
 * Tracks contain an array of Patterns, with a Title
 */

/** ZMT File Format Explained
 *
 * Data is stored in Little Endianess
 *
 *
 * ZMTn - file header with version (n)
 * Title - 12 bytes
 * Pattern count - 1 byte
 * Voice bitmap - 1 byte (low nibble) - 00001111 (all four voices have data)
 * Voice header - 4 bytes (step bitmap) 00000000 00000000 00000000 00000000 - 1 represents that the step exists
 * Step header - 1 byte (low nibble) - 00001001 (1 = note, 2 = wave, 3 = F1, 4 = F2)
 * Cells - 4 bytes (Note, W, F1, F2)
 *
 * EXAMPLE
 *
 *** file header
 * "ZMT",0x00 - Header
 * "My Track    " - Title
 * 0x02 - 2 Patterns
 *
 *** PATTERN 0
 * 0x03 - 2 populated Voices
 **
 **** PATTERN 0, VOICE 0
 * 0x11, 0x11, 0x11, 0x91 - voice header, every 4th step, with step 32 (10010001000100010001000100010001)
 * 0x01 - step header, note has data
 * 0x18 - note data for first step, C-2
 * 0x01 - step header, note has data
 * 0x18 - note data for first step, C-2
 * ... repeats for each of the "every 4" steps
 * 0x01 - step header, note has data
 * 0x6D - note data for first step, OFF
 *
**** PATTERN 0, VOICE 1
 * 0x44,0x44,0x44,0xC4 - voice header, every 4th step, starting at 3, with step 32 (11000100010001000100010001000100)
 * 0x01 - step header, note has data
 * 0x30 - note data for first step, C-4
 * 0x01 - step header, note has data
 * 0x18 - note data for first step, C-4
 * ... repeats for each of the "every 4" steps
 * 0x01 - step header, note has data
 * 0x6D - note data for first step, OFF
 *
 *** PATTERN 1
 * 0x01 - 1 populated Voices
 **
 **** pattern 0, voice 0
 * 0x91, 0x11, 0x11, 0x11 - voice header, every 4th step, with step 32 (10010001000100010001000100010001)
 * 0x01 - step header, note has data
 * 0x18 - note data for first step, C-2
 * 0x01 - step header, note has data
 * 0x18 - note data for first step, C-2
 * ... repeats for each of the "every 4" steps
 * 0x01 - step header, note has data
 * 0x6D - note data for first step, OFF
 */

#define STEPS_PER_PATTERN     32U
#define NUM_VOICES            4U
#define NUM_PATTERNS          8U
#define TRACKER_TITLE_LEN     12U

#define FRAMES_PER_QUARTER    (32U)
#define FRAMES_PER_EIGTH      (FRAMES_PER_QUARTER >> 1)
#define FRAMES_PER_SIXTEENTH  (FRAMES_PER_QUARTER >> 2)
#define FRAMES_PER_STEP       (FRAMES_PER_SIXTEENTH)

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

  FX_VOICE_SQ    = 0x10,
  FX_VOICE_TRI   = 0x11,
  FX_VOICE_SAW   = 0x12,
  FX_VOICE_NOISE = 0x13,

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
typedef uint8_t note_index_t;
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
  note_index_t note; // note index
  // note_t note; // note index - should be 8-bit, not 16-bit!!!
  waveform_t waveform;
  fx_t fx1;
  fx_t fx2;

  // values for fx
  fx_attr_t fx1_attr;
  fx_attr_t fx2_attr;
} step_t;

#define FX_ATTRS      2
#define FX_ATTR_BYTES (sizeof(fx_attr_t) * FX_ATTRS)
#define STEP_BYTES    (sizeof(step_t) - FX_ATTRS)

#define EMPTY_PATTERN 0xFD
#define EMPTY_VOICE   0xFE
#define EMPTY_STEP    0xFF

#define STEP_CELL_NOTE  0x01
#define STEP_CELL_WAVE  0x02
#define STEP_CELL_FX1   0x04
#define STEP_CELL_FX2   0x08

typedef struct {
  uint8_t index;
  step_t steps[STEPS_PER_PATTERN];
} voice_t;

typedef struct {
  // char title[TRACKER_TITLE_LEN];
  voice_t voices[NUM_VOICES];
  // TODO: add sample voice?
} pattern_t;

typedef struct {
  char title[TRACKER_TITLE_LEN + 1];
  uint8_t pattern_count;
  pattern_t* patterns[NUM_PATTERNS];
} track_t;

void zmt_process_fx(step_t *step, fx_t fx, sound_voice_t voice);
void zmt_play_step(step_t *step, sound_voice_t voice);
void zmt_play_pattern(pattern_t *pattern, uint8_t step_index);
zos_err_t zmt_pattern_load(pattern_t *pattern, zos_dev_t dev);
zos_err_t zmt_pattern_save(pattern_t *pattern, zos_dev_t dev);
zos_err_t zmt_file_load(track_t *track, const char *filename);
zos_err_t zmt_file_save(track_t *track, const char *filename);
void zmt_step_init(step_t *step, uint8_t index);
void zmt_voice_init(voice_t *voice, uint8_t index);
void zmt_pattern_init(pattern_t *pattern);
void zmt_pattern_reset(pattern_t *pattern);
void zmt_sound_off(void);
void zmt_reset(sound_volume_t vol);
uint8_t zmt_tick(pattern_t *pattern);

#define VOICEALL  (VOICE0 | VOICE1 | VOICE2 | VOICE3)

static inline void SOUND_DIV(uint16_t div) {
  zvb_peri_sound_freq_low  = div & 0xff;
  zvb_peri_sound_freq_high = (div >> 8) & 0xff;
}

static inline void SOUND_WAVE(uint8_t waveform) {
  zvb_peri_sound_wave = (waveform & 03);
}

static inline void SOUND_SELECT(uint8_t voices) {
  zvb_peri_sound_select = voices;
}

static inline void SOUND_OFF(void) {
  SOUND_SELECT(VOICEALL);
  zvb_peri_sound_freq_low  = 0x00;
  zvb_peri_sound_freq_high = 0x00;
}

static inline void SOUND_VOL(uint8_t vol) {
  zvb_peri_sound_master_vol = vol;
}

static inline void SOUND_RESET(uint8_t vol) {
  SOUND_VOL(vol);
  zvb_peri_sound_hold &= ~VOICEALL;
  SOUND_OFF();
}

#endif