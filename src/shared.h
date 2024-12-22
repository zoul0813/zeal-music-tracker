#include <zvb_hardware.h>
#include <zos_video.h>
#include "tracker.h"
#include "windows.h"

#ifndef SHARED_H
#define SHARED_H

#define DEFAULT_CURSOR (30U)

#define PATTERN_WINDOW_FG  TEXT_COLOR_LIGHT_GRAY
#define PATTERN_WINDOW_BG  TEXT_COLOR_DARK_BLUE
#define PATTERN_WINDOW_HL1 TEXT_COLOR_CYAN
#define PATTERN_WINDOW_HL2 TEXT_COLOR_YELLOW

#define DEFAULT_CURSOR_BLINK 30

typedef uint8_t (*keypress_t)(unsigned char c);
typedef void (*current_step_t)(uint8_t);
typedef void (*callback_t)(void);
typedef int (*confirm_t)(uint8_t);

typedef enum {
    VIEW_NONE,      /* 0 */
    VIEW_ARRANGER,  /* 1 */
    VIEW_PATTERN,   /* 2 */
    VIEW_HELP,      /* 3 */
    VIEW_FILE_SAVE, /* 4 */
    VIEW_FILE_LOAD, /* 5 */
    VIEW_QUIT,      /* 6 */
} View;

extern track_t track;
extern pattern_t* active_pattern;
extern uint8_t active_pattern_index;

extern keypress_t keypress_handler;
extern current_step_t current_step_handler;
extern current_step_t current_arrangement_handler;
extern callback_t close_handler;
extern confirm_t confirm_handler;

extern uint8_t dirty_track;

extern char textbuff[SCREEN_COL80_WIDTH];

int __exit(zos_err_t err);
void handle_error(zos_err_t err, char* msg, uint8_t fatal);


static inline void cursor_x(unsigned char x)
{
    zvb_peri_text_curs_x = x;
}

static inline void cursor_y(unsigned char y)
{
    zvb_peri_text_curs_y = y;
}

static inline void cursor_xy(unsigned char x, unsigned char y)
{
    cursor_x(x);
    cursor_y(y);
}

static unsigned char cursor(unsigned char onoff)
{
    static unsigned char _previous = 1;
    unsigned char old_state        = _previous;
    _previous                      = onoff;
    zvb_peri_text_curs_time        = onoff ? DEFAULT_CURSOR_BLINK : 0;
    return old_state;
}

static inline void setcolor(uint8_t fg, uint8_t bg)
{
    zvb_peri_text_color = (bg << 4 & 0xF0) | (fg & 0x0F);
}

static void print(const char* buf)
{
    while (*buf) {
        zvb_peri_text_print_char = *buf;
        buf++;
    }
}

#endif