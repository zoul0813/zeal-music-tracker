#include <stdio.h>
// #include <zgdk.h>
#include "tracker.h"

note_t NOTES[] = {
  /* Octave 0 */
  16, // C0
  17, // C#0
  18, // D0
  19, // D#0
  21, // E0
  22, // F0
  23, // F#0
  24, // G0
  26, // G#0
  27, // A0
  29, // A#0
  31, // B0

  /* Octave 1 */
  33, // C1
  35, // C#1
  37, // D1
  39, // D#1
  41, // E1
  44, // F1
  47, // F#1
  49, // G1
  52, // G#1
  55, // A1
  59, // A#1
  62, // B1

  /* Octave 2 */
  65,  // C2
  69,  // C#2
  73,  // D2
  78,  // D#2
  82,  // E2
  87,  // F2
  92,  // F#2
  98,  // G2
  104, // G#2
  110, // A2
  117, // A#2
  123, // B2

  /* Octave 3 */
  131,  // C3
  139,  // C#3
  147,  // D3
  156,  // D#3
  165,  // E3
  175,  // F3
  185,  // F#3
  196,  // G3
  208, // G#3
  220, // A3
  233, // A#3
  247, // B3

  /* Octave 4 */
  262,  // C4
  277,  // C#4
  294,  // D4
  311,  // D#4
  330,  // E4
  349,  // F4
  370,  // F#4
  392,  // G4
  415, // G#4
  440, // A4
  466, // A#4
  494, // B4

  /* Octave 5 */
  523,  // C5
  554,  // C#5
  587,  // D5
  622,  // D#5
  659,  // E5
  698,  // F5
  740,  // F#5
  784,  // G5
  830, // G#5
  880, // A5
  932, // A#5
  988, // B5

  /* Octave 6 */
  1046,  // C6
  1109,  // C#6
  1175,  // D6
  1244,  // D#6
  1319,  // E6
  1397,  // F6
  1480,  // F#6
  1568,  // G6
  1661, // G#6
  1760, // A6
  1865, // A#6
  1975, // B6

  /* Octave 7 */
  2093,  // C7
  2217,  // C#7
  2349,  // D7
  2489,  // D#7
  2637,  // E7
  2794,  // F7
  2960,  // F#7
  3136,  // G7
  3322, // G#7
  3520, // A7
  3729, // A#7
  3951, // B7

  /* Octave 8 */
  4186,  // C8
  4435,  // C#8
  4698,  // D8
  4978,  // D#8
  5274,  // E8
  5588,  // F8
  5920,  // F#8
  6272,  // G8
  6645, // G#8
  7040, // A8
  7459, // A#8
  7902, // B8

  0xFFFF, // OFF
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

zos_err_t pattern_load(pattern_t *pattern, zos_dev_t dev) {
  uint16_t size = 0;
  zos_err_t err = ERR_SUCCESS;
  for(uint8_t i = 0; i < NUM_VOICES; i++) {
    voice_t* voice = pattern->voices[i];
    size = sizeof(uint8_t);
    // text[0] = voice->voice;
    err = read(dev, &voice->voice, &size); // voice number
    if(err != ERR_SUCCESS) {
      printf("error loading pattern voice, %d (%02x)\n", err, err);
      return err;
    }

    for(uint8_t j = 0; j < STEPS_PER_PATTERN; j++) {
      size = sizeof(step_t);
      err = read(dev, &voice->steps[j], &size); // step
      if(err != ERR_SUCCESS) {
        printf("error loading pattern steps, %d (%02x) - read %d, expected %d\n", err, err, size, sizeof(step_t));
        return err;
      }
    }
  }
  return err;
}

zos_err_t pattern_save(pattern_t *pattern, zos_dev_t dev) {
  uint16_t size = 0;
  zos_err_t err = ERR_SUCCESS;
  char text[5];
  for(uint8_t i = 0; i < NUM_VOICES; i++) {
    voice_t* voice = pattern->voices[i];
    size = sizeof(uint8_t);
    text[0] = voice->voice;
    err = write(dev, text, &size); // voice number
    if(err != ERR_SUCCESS) return err;

    for(uint8_t j = 0; j < STEPS_PER_PATTERN; j++) {
      size = sizeof(step_t);
      err = write(dev, &voice->steps[j], &size); // step
      if(err != ERR_SUCCESS) return err;
    }
  }
  return err;
}

void pattern_init(pattern_t *pattern) {
  sprintf(pattern->title, "Pattern 1\x0");
  pattern->fx_counter = 0xFF;
  for(uint8_t i = 0; i < NUM_VOICES; i++) {
    pattern->voices[i]->voice = i;
    for(uint8_t j = 0; j < STEPS_PER_PATTERN; j++) {
      pattern->voices[i]->steps[j].note = NOTE_OUT_OF_RANGE;
      pattern->voices[i]->steps[j].waveform = WAVEFORM_OUT_OF_RANGE;
      pattern->voices[i]->steps[j].fx1 = FX_OUT_OF_RANGE;
      pattern->voices[i]->steps[j].fx2 = FX_OUT_OF_RANGE;
    }
  }
}
