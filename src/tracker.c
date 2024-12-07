#include <stdio.h>
#include <string.h>
// #include <zgdk.h>
#include <zvb_sound.h>
#include "tracker.h"

static uint8_t frames = 0;
static uint8_t next_step = 0;
static uint8_t last_step = 0;

/** Page Banking for Sound Peripheral */
const __sfr __banked __at(0xF0) mmu_page0_ro;

#define STEP_IS_EMPTY(step) ((step->note == NOTE_OUT_OF_RANGE) \
                            && (step->waveform == WAVEFORM_OUT_OF_RANGE) \
                            && (step->fx1 == FX_OUT_OF_RANGE) \
                            && (step->fx2 == FX_OUT_OF_RANGE))

note_t NOTES[] = {
  /* Octave 0 */
  SOUND_FREQ_TO_DIV(16), // C0
  SOUND_FREQ_TO_DIV(17), // C#0
  SOUND_FREQ_TO_DIV(18), // D0
  SOUND_FREQ_TO_DIV(19), // D#0
  SOUND_FREQ_TO_DIV(21), // E0
  SOUND_FREQ_TO_DIV(22), // F0
  SOUND_FREQ_TO_DIV(23), // F#0
  SOUND_FREQ_TO_DIV(24), // G0
  SOUND_FREQ_TO_DIV(26), // G#0
  SOUND_FREQ_TO_DIV(27), // A0
  SOUND_FREQ_TO_DIV(29), // A#0
  SOUND_FREQ_TO_DIV(31), // B0

  /* Octave 1 */
  SOUND_FREQ_TO_DIV(33), // C1
  SOUND_FREQ_TO_DIV(35), // C#1
  SOUND_FREQ_TO_DIV(37), // D1
  SOUND_FREQ_TO_DIV(39), // D#1
  SOUND_FREQ_TO_DIV(41), // E1
  SOUND_FREQ_TO_DIV(44), // F1
  SOUND_FREQ_TO_DIV(47), // F#1
  SOUND_FREQ_TO_DIV(49), // G1
  SOUND_FREQ_TO_DIV(52), // G#1
  SOUND_FREQ_TO_DIV(55), // A1
  SOUND_FREQ_TO_DIV(59), // A#1
  SOUND_FREQ_TO_DIV(62), // B1

  /* Octave 2 */
  SOUND_FREQ_TO_DIV(65),  // C2
  SOUND_FREQ_TO_DIV(69),  // C#2
  SOUND_FREQ_TO_DIV(73),  // D2
  SOUND_FREQ_TO_DIV(78),  // D#2
  SOUND_FREQ_TO_DIV(82),  // E2
  SOUND_FREQ_TO_DIV(87),  // F2
  SOUND_FREQ_TO_DIV(92),  // F#2
  SOUND_FREQ_TO_DIV(98),  // G2
  SOUND_FREQ_TO_DIV(104), // G#2
  SOUND_FREQ_TO_DIV(110), // A2
  SOUND_FREQ_TO_DIV(117), // A#2
  SOUND_FREQ_TO_DIV(123), // B2

  /* Octave 3 */
  SOUND_FREQ_TO_DIV(131),  // C3
  SOUND_FREQ_TO_DIV(139),  // C#3
  SOUND_FREQ_TO_DIV(147),  // D3
  SOUND_FREQ_TO_DIV(156),  // D#3
  SOUND_FREQ_TO_DIV(165),  // E3
  SOUND_FREQ_TO_DIV(175),  // F3
  SOUND_FREQ_TO_DIV(185),  // F#3
  SOUND_FREQ_TO_DIV(196),  // G3
  SOUND_FREQ_TO_DIV(208), // G#3
  SOUND_FREQ_TO_DIV(220), // A3
  SOUND_FREQ_TO_DIV(233), // A#3
  SOUND_FREQ_TO_DIV(247), // B3

  /* Octave 4 */
  SOUND_FREQ_TO_DIV(262),  // C4
  SOUND_FREQ_TO_DIV(277),  // C#4
  SOUND_FREQ_TO_DIV(294),  // D4
  SOUND_FREQ_TO_DIV(311),  // D#4
  SOUND_FREQ_TO_DIV(330),  // E4
  SOUND_FREQ_TO_DIV(349),  // F4
  SOUND_FREQ_TO_DIV(370),  // F#4
  SOUND_FREQ_TO_DIV(392),  // G4
  SOUND_FREQ_TO_DIV(415), // G#4
  SOUND_FREQ_TO_DIV(440), // A4
  SOUND_FREQ_TO_DIV(466), // A#4
  SOUND_FREQ_TO_DIV(494), // B4

  /* Octave 5 */
  SOUND_FREQ_TO_DIV(523),  // C5
  SOUND_FREQ_TO_DIV(554),  // C#5
  SOUND_FREQ_TO_DIV(587),  // D5
  SOUND_FREQ_TO_DIV(622),  // D#5
  SOUND_FREQ_TO_DIV(659),  // E5
  SOUND_FREQ_TO_DIV(698),  // F5
  SOUND_FREQ_TO_DIV(740),  // F#5
  SOUND_FREQ_TO_DIV(784),  // G5
  SOUND_FREQ_TO_DIV(830), // G#5
  SOUND_FREQ_TO_DIV(880), // A5
  SOUND_FREQ_TO_DIV(932), // A#5
  SOUND_FREQ_TO_DIV(988), // B5

  /* Octave 6 */
  SOUND_FREQ_TO_DIV(1046),  // C6
  SOUND_FREQ_TO_DIV(1109),  // C#6
  SOUND_FREQ_TO_DIV(1175),  // D6
  SOUND_FREQ_TO_DIV(1244),  // D#6
  SOUND_FREQ_TO_DIV(1319),  // E6
  SOUND_FREQ_TO_DIV(1397),  // F6
  SOUND_FREQ_TO_DIV(1480),  // F#6
  SOUND_FREQ_TO_DIV(1568),  // G6
  SOUND_FREQ_TO_DIV(1661), // G#6
  SOUND_FREQ_TO_DIV(1760), // A6
  SOUND_FREQ_TO_DIV(1865), // A#6
  SOUND_FREQ_TO_DIV(1975), // B6

  /* Octave 7 */
  SOUND_FREQ_TO_DIV(2093),  // C7
  SOUND_FREQ_TO_DIV(2217),  // C#7
  SOUND_FREQ_TO_DIV(2349),  // D7
  SOUND_FREQ_TO_DIV(2489),  // D#7
  SOUND_FREQ_TO_DIV(2637),  // E7
  SOUND_FREQ_TO_DIV(2794),  // F7
  SOUND_FREQ_TO_DIV(2960),  // F#7
  SOUND_FREQ_TO_DIV(3136),  // G7
  SOUND_FREQ_TO_DIV(3322), // G#7
  SOUND_FREQ_TO_DIV(3520), // A7
  SOUND_FREQ_TO_DIV(3729), // A#7
  SOUND_FREQ_TO_DIV(3951), // B7

  /* Octave 8 */
  SOUND_FREQ_TO_DIV(4186),  // C8
  SOUND_FREQ_TO_DIV(4435),  // C#8
  SOUND_FREQ_TO_DIV(4698),  // D8
  SOUND_FREQ_TO_DIV(4978),  // D#8
  SOUND_FREQ_TO_DIV(5274),  // E8
  SOUND_FREQ_TO_DIV(5588),  // F8
  SOUND_FREQ_TO_DIV(5920),  // F#8
  SOUND_FREQ_TO_DIV(6272),  // G8
  SOUND_FREQ_TO_DIV(6645), // G#8
  SOUND_FREQ_TO_DIV(7040), // A8
  SOUND_FREQ_TO_DIV(7459), // A#8
  SOUND_FREQ_TO_DIV(7902), // B8

  0x0000, // OFF
};

note_name_t NOTE_NAMES[] = {
  {'C','-','0'},
  {'C','#','0'},
  {'D','-','0'},
  {'D','#','0'},
  {'E','-','0'},
  {'F','-','0'},
  {'F','#','0'},
  {'G','-','0'},
  {'G','#','0'},
  {'A','-','0'},
  {'A','#','0'},
  {'B','-','0'},

  {'C','-','1'},
  {'C','#','1'},
  {'D','-','1'},
  {'D','#','1'},
  {'E','-','1'},
  {'F','-','1'},
  {'F','#','1'},
  {'G','-','1'},
  {'G','#','1'},
  {'A','-','1'},
  {'A','#','1'},
  {'B','-','1'},

  {'C','-','2'},
  {'C','#','2'},
  {'D','-','2'},
  {'D','#','2'},
  {'E','-','2'},
  {'F','-','2'},
  {'F','#','2'},
  {'G','-','2'},
  {'G','#','2'},
  {'A','-','2'},
  {'A','#','2'},
  {'B','-','2'},

  {'C','-','3'},
  {'C','#','3'},
  {'D','-','3'},
  {'D','#','3'},
  {'E','-','3'},
  {'F','-','3'},
  {'F','#','3'},
  {'G','-','3'},
  {'G','#','3'},
  {'A','-','3'},
  {'A','#','3'},
  {'B','-','3'},

  {'C','-','4'},
  {'C','#','4'},
  {'D','-','4'},
  {'D','#','4'},
  {'E','-','4'},
  {'F','-','4'},
  {'F','#','4'},
  {'G','-','4'},
  {'G','#','4'},
  {'A','-','4'},
  {'A','#','4'},
  {'B','-','4'},

  {'C','-','5'},
  {'C','#','5'},
  {'D','-','5'},
  {'D','#','5'},
  {'E','-','5'},
  {'F','-','5'},
  {'F','#','5'},
  {'G','-','5'},
  {'G','#','5'},
  {'A','-','5'},
  {'A','#','5'},
  {'B','-','5'},

  {'C','-','6'},
  {'C','#','6'},
  {'D','-','6'},
  {'D','#','6'},
  {'E','-','6'},
  {'F','-','6'},
  {'F','#','6'},
  {'G','-','6'},
  {'G','#','6'},
  {'A','-','6'},
  {'A','#','6'},
  {'B','-','6'},

  {'C','-','7'},
  {'C','#','7'},
  {'D','-','7'},
  {'D','#','7'},
  {'E','-','7'},
  {'F','-','7'},
  {'F','#','7'},
  {'G','-','7'},
  {'G','#','7'},
  {'A','-','7'},
  {'A','#','7'},
  {'B','-','7'},

  {'C','-','8'},
  {'C','#','8'},
  {'D','-','8'},
  {'D','#','8'},
  {'E','-','8'},
  {'F','-','8'},
  {'F','#','8'},
  {'G','-','8'},
  {'G','#','8'},
  {'A','-','8'},
  {'A','#','8'},
  {'B','-','8'},

  {'O','F','F'}
};


void zmt_process_fx(step_t *step, fx_t fx, sound_voice_t voice) {
  (void *)voice; // TODO: per channel effects
  // pattern_t *pattern = active_pattern; // TODO: gonna have more than one pattern soon, lol

  if((fx >= FX_GOTO_0) && (fx <= FX_GOTO_31)) {
    next_step = (fx - FX_GOTO_0) - 1;
    frames = (next_step % 16) - 1; // TODO: yeah?
    return;
  }

  switch(fx) {
    case FX_NOTE_OFF: { } break;
    case FX_NOTE_ON: { } break;

    case FX_VOICE_SQ:   // fall thru
    case FX_VOICE_TRI:  // fall thru
    case FX_VOICE_SAW:  // fall thru
    case FX_VOICE_NOISE: {
      SOUND_WAVE(fx);
    } break;

    case FX_COUNT_0: // fall thru
    case FX_COUNT_1: // fall thru
    case FX_COUNT_2: // fall thru
    case FX_COUNT_3: // fall thru
    case FX_COUNT_4: // fall thru
    case FX_COUNT_5: // fall thru
    case FX_COUNT_6: // fall thru
    case FX_COUNT_7: // fall thru
    case FX_COUNT_8: {
      if(step->fx1_attr == 0x00) step->fx1_attr = (fx - 0xC0);
      else step->fx1_attr--;
    } break;

    // TODO: per channel volume control?
    case FX_VOL_00_0: {
      SOUND_VOL(VOL_0);
    } break;
    case FX_VOL_12_5: {
      // TODO: 8 step volume control
      SOUND_VOL(VOL_0);
    } break;
    case FX_VOL_25_0: {
      SOUND_VOL(VOL_25);
    } break;
    case FX_VOL_37_5: {
      // TODO: 8 step volume control
      SOUND_VOL(VOL_25);
    } break;
    case FX_VOL_50_0: {
      SOUND_VOL(VOL_50);
    } break;
    case FX_VOL_62_5: {
      // TODO: 8 step volume control
      SOUND_VOL(VOL_50);
    } break;
    case FX_VOL_75_0: {
      SOUND_VOL(VOL_75);
    } break;
    case FX_VOL_87_5: {
      // TODO: 8 step volume control
      SOUND_VOL(VOL_75);
    } break;
    case FX_VOL_100: {
      SOUND_VOL(VOL_100);
    } break;
  }
}

void zmt_play_step(step_t *step, sound_voice_t voice) {
  // pattern_t *pattern = active_pattern; // TODO: gonna have more than one pattern soon, lol

  // FX1 is pre-processed
  if(step->fx1 != FX_OUT_OF_RANGE) zmt_process_fx(step, step->fx1, voice);

  SOUND_SELECT(voice);

  if(step->note != NOTE_OUT_OF_RANGE) {
    note_t note = NOTES[step->note];
    SOUND_DIV(note);
  }

  if(step->waveform != WAVEFORM_OUT_OF_RANGE) {
    waveform_t waveform = step->waveform;
    if(waveform > 0x03) waveform = WAV_SQUARE;
    SOUND_WAVE(waveform);
  }

  // only process fx2 if fx1 is not counting, or it has counted down
  if((step->fx1 >= FX_COUNT_0) && (step->fx1 <= FX_COUNT_8) && (step->fx1_attr != 0)) return;
  // FX2 is post-processed ... does this matter?
  if(step->fx2 != FX_OUT_OF_RANGE) zmt_process_fx(step, step->fx2, voice);
}

void zmt_play_pattern(pattern_t *pattern, uint8_t step_index) {
  step_t *step1 = &pattern->voices[0].steps[step_index];
  step_t *step2 = &pattern->voices[1].steps[step_index];
  step_t *step3 = &pattern->voices[2].steps[step_index];
  step_t *step4 = &pattern->voices[3].steps[step_index];

  uint8_t backup_page = mmu_page0_ro;
  zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
  zmt_play_step(step1, VOICE0);
  zmt_play_step(step2, VOICE1);
  zmt_play_step(step3, VOICE2);
  zmt_play_step(step4, VOICE3);
  zvb_map_peripheral(backup_page);
}

zos_err_t zmt_pattern_load(pattern_t *pattern, zos_dev_t dev) {
  uint16_t size = 0;
  int32_t seek_to = -1;
  zos_err_t err = ERR_SUCCESS;
  uint8_t i, j;
  unsigned char c; // used to read a single char

  for(i = 0; i < NUM_VOICES; i++) {
    voice_t* voice = &pattern->voices[i];
    size = sizeof(uint8_t);
    err = read(dev, &voice->index, &size); // voice number
    if(err != ERR_SUCCESS) {
      printf("error loading pattern voice, %d (%02x)\n", err, err);
      return err;
    }

    size = sizeof(unsigned char);
    err = read(dev, &c, &size);
    if(c != EMPTY_VOICE) {
      // seek backwards
      seek_to = -1;
      seek(dev, &seek_to, SEEK_CUR);
      for(j = 0; j < STEPS_PER_PATTERN; j++) {
        step_t *step = &voice->steps[j];
        size = sizeof(unsigned char);
        err = read(dev, &c, &size);
        if(c != EMPTY_STEP) {
          step->note = c;
          size = sizeof(step_t) - sizeof(note_index_t) - FX_ATTR_BYTES;
          err = read(dev, &step->waveform, &size); // step
          if(err != ERR_SUCCESS) {
            printf("error loading pattern steps, %d (%02x) - read %d, expected %d\n", err, err, size, sizeof(step_t));
            return err;
          }
        } else { // empty step
          zmt_step_init(step, j);
        }
      }
    } else { // empty pattern
      zmt_voice_init(voice, i);
    }
  }
  return err;
}

zos_err_t zmt_pattern_save(pattern_t *pattern, zos_dev_t dev) {
  uint16_t size = 0;
  zos_err_t err = ERR_SUCCESS;
  uint8_t i = 0, j = 0;
  unsigned char empty;
  for(i = 0; i < NUM_VOICES; i++) {
    empty = EMPTY_VOICE;
    voice_t* voice = &pattern->voices[i];
    size = sizeof(uint8_t);
    err = write(dev, &voice->index, &size); // voice number
    if(err != ERR_SUCCESS) return err;

    // Uncompressed
    // for(uint8_t j = 0; j < STEPS_PER_PATTERN; j++) {
    //   size = sizeof(step_t) - (sizeof(fx_attr_t) * FX_ATTRS);
    //   err = write(dev, &voice->steps[j], &size); // step
    //   if(err != ERR_SUCCESS) return err;
    // }

    step_t *step;
    for(j = 0; j < STEPS_PER_PATTERN; j++) {
      step = &voice->steps[j];
      if(!STEP_IS_EMPTY(step)) break;
    }

    if(j >= STEPS_PER_PATTERN) {
      // empty pattern
      size = sizeof(empty);
      err = write(dev, &empty, &size); // step
      continue;
    }

    // TODO: count number of future empty steps, then store the step byte as 0x80+N
    // ie; if the next 3 steps are empty, the next step byte is 0x83 ... which expands into 3 empty steps

    // Compressed
    empty = EMPTY_STEP;
    for(j = 0; j < STEPS_PER_PATTERN; j++) {
      step = &voice->steps[j];
      if(STEP_IS_EMPTY(step)) {
        size = sizeof(empty);
        err = write(dev, &empty, &size); // step
      } else {
        size = sizeof(step_t) - (sizeof(fx_attr_t) * FX_ATTRS);
        err = write(dev, &voice->steps[j], &size); // step
      }
      if(err != ERR_SUCCESS) return err;
    }
  }
  return err;
}

zos_err_t zmt_file_load(track_t *track, const char *filename) {
  zos_dev_t file_dev = open(filename, O_RDONLY);
  if(file_dev < 0) {
    printf("failed to open file, %d (%02x)\n", -file_dev, -file_dev);
    return -file_dev;
  } else {
    zos_err_t err;
    uint16_t size = 0;

    printf("Loading '%s' ...\n", filename);

    char textbuff[TRACKER_TITLE_LEN+1];

    size = 3;
    err = read(file_dev, textbuff, &size); // format header
    if(err != ERR_SUCCESS) {
      printf("error reading format header, %d (%02x)\n", err, err);
      return err;
    }
    printf("Format: %03s\n", textbuff);

    size = sizeof(uint8_t);
    err = read(file_dev, textbuff, &size); // version header
    if(err != ERR_SUCCESS) {
      printf("error reading version header, %d (%02x)\n", err, err);
      return err;
    }
    printf("Version: %d\n", textbuff[0]);

    size = TRACKER_TITLE_LEN;
    err = read(file_dev, textbuff, &size); // track title
    if(err != ERR_SUCCESS) {
      printf("error reading track title, %d (%02x)\n", err, err);
      return err;
    }
    memcpy(track->title, textbuff, size);
    track->title[TRACKER_TITLE_LEN] = 0x00; // NUL
    printf("Track: %12s (read: %d)\n", track->title, size);

    size = sizeof(uint8_t);
    err = read(file_dev, &track->pattern_count, &size); // pattern count
    if(err != ERR_SUCCESS) {
      printf("error reading pattern count, %d (%02x)\n", err, err);
      return err;
    }
    printf("Patterns: %d\n", track->pattern_count);

    for(uint8_t p = 0; p < track->pattern_count; p++) {
      printf("Loading pattern %d\n", p);
      err = zmt_pattern_load(track->patterns[p], file_dev);
      if(err != ERR_SUCCESS) {
        printf("error loading patterns, %d (%02x)\n", err, err);
        return err;
      }
    }

    printf("File loaded.\n\n");

    close(file_dev);
  }
  return ERR_SUCCESS;
}

zos_err_t zmt_file_save(track_t *track, const char *filename) {
  zos_dev_t file_dev = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
  if(file_dev < 0) {
    printf("failed to open file for savings, %d (%02x)", -file_dev, -file_dev);
    return -file_dev;
  } else {
    printf("Saving '%s' ...\n", filename);
    zos_err_t err;
    uint16_t size = 0;

    char textbuff[TRACKER_TITLE_LEN+1];

    size = 3;
    err = write(file_dev, "ZMT", &size); // format header
    if(err != ERR_SUCCESS) {
      printf("error saving format header, %d (%02x)\n", err, err);
      // exit(err);
      return err;
    }

    size = sizeof(uint8_t);
    textbuff[0] =  0;
    err = write(file_dev, textbuff, &size); // version header
    if(err != ERR_SUCCESS) {
      printf("error saving version header, %d (%02x)\n", err, err);
      // exit(err);
      return err;
    }

    size = TRACKER_TITLE_LEN;
    sprintf(textbuff, "%-.12s", track->title);
    err = write(file_dev, textbuff, &size); // track title
    if(err != ERR_SUCCESS) {
      printf("error saving title length, %d (%02x)\n", err, err);
      // exit(err);
      return err;
    }

    size = sizeof(uint8_t);
    err = write(file_dev, &track->pattern_count, &size); // pattern count
    if(err != ERR_SUCCESS) {
      printf("error saving pattern count, %d (%02x)\n", err, err);
      // exit(err);
      return err;
    }

    for(uint8_t p = 0; p < track->pattern_count; p++) {
      printf("Writing pattern %d\n", p);
      err = zmt_pattern_save(track->patterns[p], file_dev);
      if(err != ERR_SUCCESS) {
      printf("error saving patterns, %d (%02x)\n", err, err);
      // exit(err);
      return err;
    }
    }
    printf("File saved.\n");
    close(file_dev);
  }
  return ERR_SUCCESS;
}

void zmt_step_init(step_t *step, uint8_t index) {
  (void *)index; // unreferenced
  step->note = NOTE_OUT_OF_RANGE;
  step->waveform = WAVEFORM_OUT_OF_RANGE;
  step->fx1 = FX_OUT_OF_RANGE;
  step->fx2 = FX_OUT_OF_RANGE;

  // we don't talk about bruno
  step->fx1_attr = 0x00;
  step->fx2_attr = 0x00;
}

void zmt_voice_init(voice_t *voice, uint8_t index) {
  uint8_t i;
  voice->index = index;
  for(i = 0; i < STEPS_PER_PATTERN; i++) {
    step_t *step = &voice->steps[i];
    zmt_step_init(step, i);
  }
}

void zmt_pattern_init(pattern_t *pattern) {
  uint8_t i;
  for(i = 0; i < NUM_VOICES; i++) {
    voice_t *voice = &pattern->voices[i];
    zmt_voice_init(voice, i);
  }
}

void zmt_pattern_reset(pattern_t *pattern) {
  frames = 0;
  next_step = 0;
  for(uint8_t s = 0; s < STEPS_PER_PATTERN; s++) {
    for(uint8_t v = 0; v < NUM_VOICES; v++) {
      pattern->voices[v].steps[s].fx1_attr = 0x00;
      pattern->voices[v].steps[s].fx2_attr = 0x00;
    }
  }
}

uint8_t zmt_tick(pattern_t *pattern) {
  if(frames > FRAMES_PER_QUARTER) frames = 0;
  // if(frames % FRAMES_PER_EIGTH == 0) { }
  if(frames % FRAMES_PER_SIXTEENTH == 0) {
    // color_step(last_step, VOICE_WINDOW_FG);
    last_step = next_step;

    // color_step(next_step, VOICE_WINDOW_HL1);
    zmt_play_pattern(pattern, next_step);

    next_step++;
    if(next_step >= STEPS_PER_PATTERN) next_step = 0;
  }
  frames++;

  return next_step;
}

void zmt_sound_off(void) {
  uint8_t backup_page = mmu_page0_ro;
  zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
  SOUND_OFF();
  zvb_map_peripheral(backup_page);
}

void zmt_reset(sound_volume_t vol) {
  uint8_t backup_page = mmu_page0_ro;
  zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
  SOUND_RESET(vol);
  zvb_map_peripheral(backup_page);
}