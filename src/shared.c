#include <stdio.h>
#include <zos_sys.h>
#include <zos_vfs.h>
#include <zos_video.h>
#include "shared.h"

char textbuff[SCREEN_COL80_WIDTH];

pattern_t pattern0;
pattern_t pattern1;
pattern_t pattern2;
pattern_t pattern3;
pattern_t pattern4;
pattern_t pattern5;
pattern_t pattern6;
pattern_t pattern7;
pattern_t pattern8;
pattern_t pattern9;
pattern_t patternA;
pattern_t patternB;
pattern_t patternC;
pattern_t patternD;
pattern_t patternE;
pattern_t patternF;
const size_t PATTERN_SIZE = sizeof(pattern_t);

track_t track = {
  .title = "Track 1",
  .patterns = {
    &pattern0,
    &pattern1,
    &pattern2,
    &pattern3,
    &pattern4,
    &pattern5,
    &pattern6,
    &pattern7,
    &pattern8,
    &pattern9,
    &patternA,
    &patternB,
    &patternC,
    &patternD,
    &patternE,
    &patternF,
  }
};
const size_t TRACK_SIZE = sizeof(track);

pattern_t* active_pattern = NULL;
uint8_t active_pattern_index = 0;

int __exit(zos_err_t err) {
  zmt_reset(VOL_0);
  if(err == ERR_SUCCESS) err = ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
  exit(err);
  return err;
}

void handle_error(zos_err_t err, char *msg, uint8_t fatal) {
  if(err != ERR_SUCCESS) {
    cursor_xy(2,20);
    printf("failed to %s, %d (%02x)\n", msg, err, err);
    if(fatal) __exit(err);
  }
}