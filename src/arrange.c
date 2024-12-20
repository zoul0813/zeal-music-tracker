#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <zos_keyboard.h>
#include "tracker.h"
#include "shared.h"
#include "windows.h"
#include "arrange.h"

#define ARRANGEMENT_COL (8U)
#define ARRANGEMENT_ROW (8U)
#define ARRANGEMENT_LEN (4U)

uint8_t arrange_active_cell           = 0;
uint8_t arrange_active_step           = 0;
arrangement_t* arrange_last_step_edit = NULL;
uint8_t arrange_previous_step         = 0;

#define WINDOW_Y (5U)
#define WINDOW_W (((ARRANGEMENT_COL * (ARRANGEMENT_LEN + 1)) - 1) + 4)
#define WINDOW_X ((SCREEN_COL80_WIDTH - WINDOW_W) / 2)
#define WINDOW_H ((NUM_ARRANGEMENTS / 8) + 3)
window_t win_Arrange = {
    .x     = WINDOW_X, // (SCREEN_COL80_WIDTH - ((ARRANGEMENT_LEN * 8) + 4)) / 2,
    .y     = WINDOW_Y,
    .w     = WINDOW_W, // ((ARRANGEMENT_COL * (ARRANGEMENT_LEN + 1)) - 1) + 4,
    .h     = WINDOW_H,
    .flags = WIN_BORDER | WIN_SHADOW,
    .title = "Arranger",
    .fg    = PATTERN_WINDOW_FG,
    .bg    = PATTERN_WINDOW_BG,
};

window_t win_Settings = {
    .x     = WINDOW_X,
    .y     = WINDOW_Y + WINDOW_H + 3,
    .w     = WINDOW_W,
    .h     = WINDOW_H,
    .flags = WIN_BORDER | WIN_SHADOW,
    .title = "Settings",
    .fg    = PATTERN_WINDOW_FG,
    .bg    = PATTERN_WINDOW_BG,
};

#define STEP_XY(step)                                \
    uint8_t x  = (step % 8) * (ARRANGEMENT_LEN + 1); \
    uint8_t y  = (step >> 3) + 1;                    \
    x         += win_Arrange.x + 2;                  \
    y         += win_Arrange.y + 1;

void arrange_refresh_step(uint8_t step_index)
{
    arrangement_t* a = &track.arrangement[step_index];
    STEP_XY(step_index);
    uint8_t width = 4;

    if (a->pattern_index == ARRANGEMENT_OUT_OF_RANGE) {
        sprintf(&textbuff[0], "- ");
    } else {
        sprintf(&textbuff[0], "%01X ", a->pattern_index & 0x0F);
    }
    if (a->fx == FX_OUT_OF_RANGE) {
        sprintf(&textbuff[2], "--");
    } else {
        sprintf(&textbuff[2], "%02X", a->fx);
    }

    text_map_vram();
    for (uint8_t i = 0; i < width; i++) {
        SCR_TEXT[y][x + i] = textbuff[i];
    }
    text_demap_vram();
}

void arrange_update_cell(int8_t amount)
{
    arrangement_t* a = &track.arrangement[arrange_active_step];
    STEP_XY(arrange_active_step);
    uint8_t width = 1;
    dirty_track = 1;

    switch (arrange_active_cell) {
        case Cell_Pattern: {
            if (a->pattern_index == ARRANGEMENT_OUT_OF_RANGE) {
                if (amount > 0) {
                    a->pattern_index = 0;
                } else {
                    a->pattern_index = NUM_PATTERNS - 1;
                }
            } else {
                a->pattern_index += amount;
            }
            if (a->pattern_index >= NUM_PATTERNS)
                a->pattern_index = ARRANGEMENT_OUT_OF_RANGE;
        } break;
        case Cell_Effect: {
            if (amount > 1)
                amount = 16;
            if (amount < -1)
                amount = -16;
            a->fx += amount;
        }
    }

    switch (arrange_active_cell) {
        case Cell_Pattern: {
            if (a->pattern_index == ARRANGEMENT_OUT_OF_RANGE) {
                sprintf(textbuff, "-");
            } else {
                sprintf(textbuff, "%01X", a->pattern_index & 0x0F);
            }
        } break;
        case Cell_Effect: {
            x     += 2;
            width  = 2;
            if (a->fx == FX_OUT_OF_RANGE) {
                sprintf(textbuff, "--");
            } else {
                sprintf(textbuff, "%02X", a->fx);
            }
        } break;
    }

    text_map_vram();
    for (uint8_t i = 0; i < width; i++) {
        SCR_TEXT[y][x + i] = textbuff[i];
    }
    text_demap_vram();
}

void arrange_color_cell(uint8_t step_index, uint8_t cell_index, uint8_t color)
{
    STEP_XY(step_index);
    uint8_t width = 1;

    switch (cell_index) {
        case Cell_Effect: {
            x     += 2;
            width  = 2;
        } break;
    }
    text_map_vram();
    for (uint8_t i = 0; i < width; i++) {
        SCR_COLOR[y][x + i] = color;
    }
    text_demap_vram();
}

void arrange_color_step(uint8_t step_index, uint8_t color)
{
    STEP_XY(step_index);

    text_map_vram();
    SCR_COLOR[y][x]     = color;
    SCR_COLOR[y][x + 2] = color;
    SCR_COLOR[y][x + 3] = color;
    text_demap_vram();
}

void arrange_refresh_steps(void)
{
    window_gotoxy(&win_Arrange, 0, 0);

    uint8_t c = 0, r = 0, i = 0;
    // heading
    window_puts(&win_Arrange, " ");
    for (i = 0; i < 8; i++) {
        window_puts_color(&win_Arrange, "P FX ", COLOR(TEXT_COLOR_WHITE, PATTERN_WINDOW_BG));
    }

    // arrangement steps
    for (i = 0; i < NUM_ARRANGEMENTS; i++) {
        arrange_refresh_step(i);
    }
}

void arrange_show(uint8_t index)
{
    index; // unreferenced, ignored???
    window(&win_Arrange);
    window(&win_Settings);
    sprintf(textbuff, "%03u", track.tempo);
    window_puts(&win_Settings, "Tempo: ");
    window_puts(&win_Settings, textbuff);

    arrange_refresh_steps();
    arrange_color_step(arrange_active_step, COLOR(PATTERN_WINDOW_HL1, win_Arrange.bg));
    arrange_color_cell(arrange_active_step, arrange_active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
}

uint8_t arrange_keypress_handler(unsigned char key)
{
    switch (key) {
        case KB_LEFT_ARROW: {
            arrange_update_cell(-1);
            arrange_last_step_edit = &track.arrangement[arrange_active_step];
        } break;
        case KB_RIGHT_ARROW: {
            arrange_update_cell(1);
            arrange_last_step_edit = &track.arrangement[arrange_active_step];
        } break;
        case KB_PG_DOWN: {
            arrange_update_cell(-2);
            arrange_last_step_edit = &track.arrangement[arrange_active_step];
        } break;
        case KB_PG_UP: {
            arrange_update_cell(2);
            arrange_last_step_edit = &track.arrangement[arrange_active_step];
        } break;
        case KB_INSERT: {
            memcpy(&track.arrangement[arrange_active_step], arrange_last_step_edit, sizeof(arrangement_t));
            arrange_refresh_step(arrange_active_step);
            dirty_track = 1;
        } break;
        case KB_DELETE: {
            track.arrangement[arrange_active_step].pattern_index = ARRANGEMENT_OUT_OF_RANGE;
            track.arrangement[arrange_active_step].fx            = FX_OUT_OF_RANGE;
            arrange_refresh_step(arrange_active_step);
            dirty_track = 1;
        } break;

        /* Tempo */
        case KB_KEY_R: {
            if (track.tempo > 8)
                ;
            track.tempo -= 4;
            sprintf(textbuff, "%03u", track.tempo);
            window_gotoxy(&win_Settings, 7, 0);
            window_puts(&win_Settings, textbuff);
        } break;
        case KB_KEY_T: {
            if (track.tempo < 128)
                track.tempo += 4;
            sprintf(textbuff, "%03u", track.tempo);
            window_gotoxy(&win_Settings, 7, 0);
            window_puts(&win_Settings, textbuff);
        } break;

        case KB_UP_ARROW: {
            arrange_color_step(arrange_active_step, COLOR(PATTERN_WINDOW_FG, win_Arrange.bg));
            if (arrange_active_step > 0) {
                arrange_active_step--;
            } else {
                arrange_active_step = (NUM_ARRANGEMENTS - 1);
            }
            arrange_color_step(arrange_active_step, COLOR(PATTERN_WINDOW_HL1, win_Arrange.bg));
            arrange_color_cell(arrange_active_step, arrange_active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
        } break;
        case KB_DOWN_ARROW: {
            arrange_color_step(arrange_active_step, COLOR(PATTERN_WINDOW_FG, win_Arrange.bg));
            arrange_active_step++;
            if (arrange_active_step > (NUM_ARRANGEMENTS - 1))
                arrange_active_step = 0;
            arrange_color_step(arrange_active_step, COLOR(PATTERN_WINDOW_HL1, win_Arrange.bg));
            arrange_color_cell(arrange_active_step, arrange_active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
        } break;
        case KB_HOME: {
            arrange_color_step(arrange_active_step, COLOR(PATTERN_WINDOW_FG, win_Arrange.bg));
            arrange_active_step = 0;
            arrange_color_step(arrange_active_step, COLOR(PATTERN_WINDOW_HL1, win_Arrange.bg));
            arrange_color_cell(arrange_active_step, arrange_active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
        } break;
        case KB_END: {
            arrange_color_step(arrange_active_step, COLOR(PATTERN_WINDOW_FG, win_Arrange.bg));
            arrange_active_step = NUM_ARRANGEMENTS - 1;
            arrange_color_step(arrange_active_step, COLOR(PATTERN_WINDOW_HL1, win_Arrange.bg));
            arrange_color_cell(arrange_active_step, arrange_active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
        } break;

        case KB_KEY_TAB: {
            arrange_color_cell(arrange_active_step, arrange_active_cell, COLOR(PATTERN_WINDOW_HL1, PATTERN_WINDOW_BG));
            arrange_active_cell++;
            if (arrange_active_cell > 1)
                arrange_active_cell = 0;
            arrange_color_cell(arrange_active_step, arrange_active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
        } break;

        default: {
            return 0; // unhandled
        }
    }
    return 1;
}

void arrange_current_step_handler(uint8_t current_step)
{
    current_step; // unreferenced
                  // do nothing
}

void arrange_current_arrangement_handler(uint8_t current_arragement)
{
    if (arrange_previous_step == arrange_active_step) {
        arrange_color_step(arrange_previous_step, COLOR(PATTERN_WINDOW_HL1, win_Arrange.bg));
    } else {
        arrange_color_step(arrange_previous_step, COLOR(win_Arrange.fg, win_Arrange.bg));
    }
    arrange_color_step(current_arragement, COLOR(PATTERN_WINDOW_HL2, win_Arrange.bg));
    arrange_previous_step = current_arragement;
}