#include <zgdk.h>

#include "tracker.h"

zos_err_t pattern_load(pattern_t *pattern, zos_dev_t dev) {
  uint16_t size = 0;
  zos_err_t err = ERR_SUCCESS;
  for(uint8_t i = 0; i < NUM_VOICES; i++) {
    voice_t* voice = pattern->voices[i];
    size = sizeof(uint8_t);
    // text[0] = voice->voice;
    err = read(dev, &voice->voice, &size); // voice number
    if(err != ERR_SUCCESS) return err;

    for(uint8_t j = 0; j < STEPS_PER_PATTERN; j++) {
      size = sizeof(step_t);
      err = read(dev, &voice->steps[j], &size); // step
      if(err != ERR_SUCCESS) return err;
    }
  }
  return err;
}

zos_err_t pattern_save(pattern_t *pattern, zos_dev_t dev) {
  uint16_t size = 0;
  zos_err_t err = ERR_SUCCESS;
  char text[10];
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