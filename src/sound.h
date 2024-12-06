#include <zvb_hardware.h>
#include <zvb_sound.h>

#ifndef SOUND_H
#define SOUND_H

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