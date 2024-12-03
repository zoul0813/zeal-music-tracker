#include <stdio.h>
// #include <zgdk.h>
#include "tracker.h"

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
  for(uint8_t i = 0; i < NUM_VOICES; i++) {
    pattern->voices[i]->voice = i;
    for(uint8_t j = 0; j < STEPS_PER_PATTERN; j++) {
      pattern->voices[i]->steps[j].freq = 0xFFFF;
      pattern->voices[i]->steps[j].waveform = 0xFF;
      pattern->voices[i]->steps[j].fx1 = 0xFF;
      pattern->voices[i]->steps[j].fx2 = 0xFF;
    }
  }
}
