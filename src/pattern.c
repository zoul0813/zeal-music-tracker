#include <core.h>
#include <zos_keyboard.h>
#include <windows.h>

#include <zgdk/sound/tracker.h>
#include "shared.h"
#include "pattern.h"
#include "dialogs/confirm_dialog.h"


voice_t* active_voice      = NULL;
uint8_t active_voice_index = 0;
uint8_t active_cell        = 0;
uint8_t active_step        = 0;
step_t* last_step_edit     = NULL;
uint8_t previous_step      = 0;

static uint8_t recording;
static uint8_t input_octave = 4;
/* A held key owns its voice until release, even after selecting another voice. */
static uint8_t voice_keys[NUM_VOICES];

window_t win_Indicators = {
    .x     = 2,
    .y     = PATTERN_WIN_Y,
    .w     = 2,
    .h     = STEPS_PER_PATTERN,
    .fg    = TEXT_COLOR_LIGHT_GRAY,
    .bg    = TEXT_COLOR_DARK_GRAY,
    .fg_highlight = TEXT_COLOR_WHITE,
    .flags = 0,
};

window_t win_Pattern1 = {.x     = PATTERN_WIN_X + (PATTERN_WIN_WIDTH * 0),
                         .y     = PATTERN_WIN_Y,
                         .h     = 1 + STEPS_PER_PATTERN + 2,
                         .w     = PATTERN_WIN_WIDTH,
                         .fg    = PATTERN_WINDOW_FG,
                         .bg    = PATTERN_WINDOW_BG,
                         .fg_highlight = PATTERN_WINDOW_HL1,
                         .flags = WIN_BORDER | WIN_SHADOW,
                         .title = "Voice 1"};

window_t win_Pattern2 = {.x     = PATTERN_WIN_X + (PATTERN_WIN_WIDTH * 1) + 1 + 1,
                         .y     = PATTERN_WIN_Y,
                         .h     = 1 + STEPS_PER_PATTERN + 2,
                         .w     = PATTERN_WIN_WIDTH,
                         .fg    = PATTERN_WINDOW_FG,
                         .bg    = PATTERN_WINDOW_BG,
                         .fg_highlight = PATTERN_WINDOW_HL1,
                         .flags = WIN_BORDER | WIN_SHADOW,
                         .title = "Voice 2"};

window_t win_Pattern3 = {.x     = PATTERN_WIN_X + (PATTERN_WIN_WIDTH * 2) + 2 + 2,
                         .y     = PATTERN_WIN_Y,
                         .h     = 1 + STEPS_PER_PATTERN + 2,
                         .w     = PATTERN_WIN_WIDTH,
                         .fg    = PATTERN_WINDOW_FG,
                         .bg    = PATTERN_WINDOW_BG,
                         .fg_highlight = PATTERN_WINDOW_HL1,
                         .flags = WIN_BORDER | WIN_SHADOW,
                         .title = "Voice 3"};

window_t win_Pattern4 = {.x     = PATTERN_WIN_X + (PATTERN_WIN_WIDTH * 3) + 3 + 3,
                         .y     = PATTERN_WIN_Y,
                         .h     = 1 + STEPS_PER_PATTERN + 2,
                         .w     = PATTERN_WIN_WIDTH,
                         .fg    = PATTERN_WINDOW_FG,
                         .bg    = PATTERN_WINDOW_BG,
                         .fg_highlight = PATTERN_WINDOW_HL1,
                         .flags = WIN_BORDER | WIN_SHADOW,
                         .title = "Voice 4"};

window_t* windows[NUM_VOICES] = {
    &win_Pattern1,
    &win_Pattern2,
    &win_Pattern3,
    &win_Pattern4,
};

void pattern_show_input_status(void)
{
    if (active_view != VIEW_PATTERN || keypress_handler != &pattern_keypress_handler)
        return;
    uint8_t held = 0;
    for (uint8_t i = 0; i < NUM_VOICES; i++)
        held += voice_keys[i] != 0;
    char digit[2] = {0, 0};
    text_map_vram();
    cursor_xy(18, 1);
    setcolor(recording ? TEXT_COLOR_YELLOW : TEXT_COLOR_BLACK, TEXT_COLOR_LIGHT_GRAY);
    print(recording ? "RECORD   " : playing ? "PLAY     " : "AUDITION ");
    print("Oct:");
    digit[0] = '0' + input_octave;
    print(digit);
    print(" Held:");
    digit[0] = '0' + held;
    print(digit);
    print(" [R] Record [-/=] Octave   ");
    text_demap_vram();
}

static void pattern_sound_step(step_t* step, uint8_t voice_index)
{
    uint8_t backup_page = zvb_config_dev_idx;
    zvb_map_peripheral(ZVB_PERI_SOUND_IDX);
    zmt_play_step(step, 1U << voice_index);
    zvb_map_peripheral(backup_page);
}

void pattern_release_key(uint8_t key)
{
    for (uint8_t i = 0; i < NUM_VOICES; i++) {
        if (voice_keys[i] && voice_keys[i] == key) {
            step_t off;
            zmt_step_init(&off, 0);
            off.note = NOTE_OFF;
            pattern_sound_step(&off, i);
            voice_keys[i] = 0;
            pattern_show_input_status();
        }
    }
}

void pattern_release_all(void)
{
    for (uint8_t i = 0; i < NUM_VOICES; i++)
        pattern_release_key(voice_keys[i]);
}

void pattern_disarm_recording(void)
{
    pattern_release_all();
    recording = 0;
}

static void preview_effect(step_t* preview, fx_t fx)
{
    if (fx >= FX_VOICE_SQ && fx <= FX_VOICE_NOISE)
        preview->waveform = fx - FX_VOICE_SQ;
    else if (fx >= FX_VOL_00_0 && fx <= FX_VOL_100)
        preview->fx1 = fx;
}

static void pattern_audition(uint8_t voice_index, uint8_t note)
{
    step_t preview;
    zmt_step_init(&preview, 0);
    preview.note = note;
    preview.waveform = WAV_SQUARE;
    preview.fx1 = FX_VOL_75_0;
    /* Resolve sound settings without executing transport FX or mutating the song. */
    for (uint8_t row = 0; row <= active_step; row++) {
        step_t* step = &active_pattern->voices[voice_index].steps[row];
        preview_effect(&preview, step->fx1);
        if (step->waveform != WAVEFORM_OUT_OF_RANGE)
            preview.waveform = step->waveform <= 3 ? step->waveform : WAV_SQUARE;
        preview_effect(&preview, step->fx2);
    }
    pattern_sound_step(&preview, voice_index);
}

static uint8_t piano_offset(uint8_t key)
{
    const char* keys = "zsxdcvgbhnjm";
    for (uint8_t i = 0; i < 12; i++) {
        if (keys[i] == key)
            return i;
    }
    return 0xFF;
}

static void pattern_select_voice(uint8_t voice_index)
{
    window_active(windows[active_voice_index], 0);
    active_voice_index = voice_index;
    active_voice       = &active_pattern->voices[active_voice_index];
    window_active(windows[active_voice_index], 1);
}

void pattern_reset_state(void)
{
    pattern_disarm_recording();
    input_octave = 4;
    active_pattern_index = zmt_pattern_set(&track, 0);
    active_pattern       = track.patterns[active_pattern_index];
    active_voice_index   = 0;
    active_voice         = &active_pattern->voices[active_voice_index];
    active_cell          = 0;
    active_step          = 0;
    last_step_edit       = NULL;
    previous_step        = 0;
}

void pattern_update_cell(voice_t* voice, int8_t amount)
{
    step_t* step = &voice->steps[active_step];
    window_t* w  = windows[active_voice_index];
    dirty_track  = 1;
    switch (active_cell) {
        case Cell_Frequency:
            if (amount > 1)
                amount = 12;
            if (amount < -1)
                amount = -12;

            if (step->note >= NOTE_OUT_OF_RANGE) {
                if (amount > 0) {
                    step->note = NOTE_MIDDLE_C;
                } else {
                    step->note = NUM_NOTES - 1;
                }
            } else {
                step->note += amount;
            }
            if (step->note >= NUM_NOTES) {
                step->note = NOTE_OUT_OF_RANGE;
            }
            window_gotoxy(w, CELL_OFFSET_FREQ, active_step + 1);
            if (step->note == NOTE_OUT_OF_RANGE) {
                window_puts_color(w, "---", COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
            } else {
                mem_cpy(textbuff, NOTE_NAMES[step->note], 3);
                textbuff[3] = 0;
                window_puts_color(w, textbuff, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
            }
            break;
        case Cell_Waveform:
            if (amount < 0 && step->waveform == 0xFF) {
                step->waveform = 0x04;
            }
            step->waveform += amount;
            if (step->waveform > 0x03) {
                step->waveform = 0xFF;
            }
            itoa_pad(step->waveform & 0x0F, textbuff, 16, 'A', '0', 1);
            window_gotoxy(w, CELL_OFFSET_WAVE, active_step + 1);
            window_puts_color(w, textbuff, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
            break;
        case Cell_Effect1:
            if (amount > 1)
                amount = 16;
            if (amount < -1)
                amount = -16;
            step->fx1 += amount;
            itoa_pad(step->fx1, textbuff, 16, 'A', '0', 2);
            window_gotoxy(w, CELL_OFFSET_F1, active_step + 1);
            window_puts_color(w, textbuff, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
            break;
        case Cell_Effect2:
            if (amount > 1)
                amount = 16;
            if (amount < -1)
                amount = -16;
            step->fx2 += amount;
            itoa_pad(step->fx2, textbuff, 16, 'A', '0', 2);
            window_gotoxy(w, CELL_OFFSET_F2, active_step + 1);
            window_puts_color(w, textbuff, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
            break;
    }
}

void pattern_color_step(uint8_t step_index, uint8_t color)
{
    uint8_t x1  = windows[0]->x + 2;
    uint8_t x2  = windows[1]->x + 2;
    uint8_t x3  = windows[2]->x + 2;
    uint8_t x4  = windows[3]->x + 2;
    uint8_t y   = windows[0]->y + 1 + step_index + 1;
    uint8_t clr = COLOR(color, windows[0]->bg); //(bg << 4) | (color & 0x0F);

    text_map_vram();
    // MAGIC: 12 = magic length of the step text :)
    SCR_COLOR[y][2] = COLOR(color, TEXT_COLOR_DARK_GRAY);
    SCR_COLOR[y][3] = COLOR(color, TEXT_COLOR_DARK_GRAY);
    for (uint8_t i = 0; i < 11; i++) {
        SCR_COLOR[y][x1 + i] = clr;
        SCR_COLOR[y][x2 + i] = clr;
        SCR_COLOR[y][x3 + i] = clr;
        SCR_COLOR[y][x4 + i] = clr;
    }
    text_demap_vram();
}

void pattern_color_cell(uint8_t step_index, uint8_t cell_index, uint8_t color)
{
    uint8_t y      = windows[active_voice_index]->y + 1 + step_index + 1;
    uint8_t bg     = windows[active_voice_index]->bg;
    uint8_t offset = 0;
    uint8_t x      = windows[active_voice_index]->x + 1;
    uint8_t width  = 1;
    uint8_t i      = 0;

    switch (cell_index) {
        case 0: offset = CELL_OFFSET_FREQ; break;
        case 1: offset = CELL_OFFSET_WAVE; break;
        case 2: offset = CELL_OFFSET_F1; break;
        case 3: offset = CELL_OFFSET_F2; break;
    }

    x += offset;

    text_map_vram();
    switch (active_cell) {
        case Cell_Frequency: width = 3; break;
        case Cell_Waveform: break;
        case Cell_Effect1: width = 2; break;
        case Cell_Effect2: width = 2; break;
    }
    for (i = 0; i < width; i++) {
        SCR_COLOR[y][x + i] = color;
    }
    text_demap_vram();
}

void pattern_refresh_step(uint8_t voice_index, uint8_t step_index)
{
    window_t* w    = windows[voice_index];
    voice_t* voice = &active_pattern->voices[voice_index];
    step_t* step   = &voice->steps[step_index];

    window_gotoxy(w, 0, step_index + 1);

    if ((step_index == 0) || (step_index % 4 == 0)) {
        window_putc(w, 249);
    } else {
        window_putc(w, ' ');
    }

    if (step->note == NOTE_OUT_OF_RANGE) {
        window_puts(w, "---");
    } else {
        mem_cpy(textbuff, NOTE_NAMES[step->note], 3);
        textbuff[3] = 0;
        window_puts(w, textbuff);
    }

    if (step->waveform == 0xFF) {
        window_puts(w, " - ");
    } else {
        textbuff[0] = ' ';
        itoa_pad(step->waveform & 0x0F, &textbuff[1], 16, 'A', '0', 1);
        textbuff[2] = ' ';
        textbuff[3] = 0;
        window_puts(w, textbuff);
    }

    if (step->fx1 == 0xFF) {
        window_puts(w, "-- ");
    } else {
        itoa_pad(step->fx1, textbuff, 16, 'A', '0', 2);
        textbuff[2] = ' ';
        textbuff[3] = 0;
        window_puts(w, textbuff);
    }

    if (step->fx2 == 0xFF) {
        window_puts(w, "-- ");
    } else {
        itoa_pad(step->fx2, textbuff, 16, 'A', '0', 2);
        textbuff[2] = ' ';
        textbuff[3] = 0;
        window_puts(w, textbuff);
    }
}

void pattern_refresh_steps(void)
{
    uint8_t i = 0, j = 0;
    for (i = 0; i < NUM_VOICES; i++) {
        window_t* w = windows[i];
        window(w);

        voice_t* voice = &active_pattern->voices[i];
        window_puts_color(w, " No. W F1 F2\n", COLOR(TEXT_COLOR_WHITE, PATTERN_WINDOW_BG));
        for (j = 0; j < STEPS_PER_PATTERN; j++) {
            pattern_refresh_step(i, j);
        }
    }

    pattern_color_step(0, PATTERN_WINDOW_HL1);
    pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
}

void pattern_show(uint8_t index)
{
    active_pattern_index = zmt_pattern_set(&track, index);
    active_pattern       = track.patterns[active_pattern_index];

    // pattern step indicators
    window(&win_Indicators);
    window_gotoxy(&win_Indicators, 0, 0);
    textbuff[0] = 'P';
    itoa_pad(active_pattern_index & 0x0F, &textbuff[1], 16, 'A', '0', 1);
    window_puts(&win_Indicators, textbuff);
    window_gotoxy(&win_Indicators, 0, 2);
    for (uint8_t i = 0; i <= FX_GOTO_31 - FX_GOTO_0; i++) {
        itoa_pad(i + FX_GOTO_0, textbuff, 16, 'A', '0', 2);
        window_puts(&win_Indicators, textbuff);
    }

    active_voice_index = active_voice_index;
    active_voice       = &active_pattern->voices[active_voice_index];
    pattern_refresh_steps();
    window_active(windows[active_voice_index], 1);
    pattern_show_input_status();
}

void pattern_current_step_handler(uint8_t current_step)
{
    pattern_color_step(previous_step, PATTERN_WINDOW_FG);
    pattern_color_step(current_step, PATTERN_WINDOW_HL2);
    previous_step = current_step;
}

static uint8_t pattern_is_referenced(uint8_t pattern_index)
{
    for (uint8_t i = 0; i < NUM_ARRANGEMENTS; i++) {
        if (track.arrangement[i].pattern_index == pattern_index)
            return 1;
    }
    return 0;
}

int clear_pattern_handler(uint8_t confirmed)
{
    if (confirmed != ERR_SUCCESS)
        return ERR_FAILURE;

    zmt_pattern_init(active_pattern);

    dirty_track = 1;
    window_gotoxy(&win_Indicators, 0, 0);
    textbuff[0] = 'P';
    itoa_pad(active_pattern_index & 0x0F, &textbuff[1], 16, 'A', '0', 1);
    window_puts(&win_Indicators, textbuff);

    pattern_refresh_steps();
    pattern_color_step(active_step, PATTERN_WINDOW_HL1);
    pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));

    return ERR_SUCCESS;
}

int delete_pattern_handler(uint8_t confirmed)
{
    if (confirmed != ERR_SUCCESS)
        return ERR_FAILURE;
    if (pattern_is_referenced(active_pattern_index))
        return ERR_FAILURE;

    uint8_t deleted_pattern_index = active_pattern_index;

    // shift everything down...
    uint8_t i;
    for (i = active_pattern_index + 1; i < NUM_PATTERNS; i++) {
        pattern_t* p_dest = track.patterns[i - 1];
        pattern_t* p_src  = track.patterns[i];
        mem_cpy(p_dest, p_src, sizeof(pattern_t));
    }

    for (i = 0; i < NUM_ARRANGEMENTS; i++) {
        arrangement_t* arrangement = &track.arrangement[i];
        if (arrangement->pattern_index != ARRANGEMENT_OUT_OF_RANGE &&
            arrangement->pattern_index > deleted_pattern_index) {
            arrangement->pattern_index--;
        }
    }

    track.pattern_count--;
    if (active_pattern_index >= track.pattern_count) {
        active_pattern_index = track.pattern_count - 1;
    }
    active_pattern_index = zmt_pattern_set(&track, active_pattern_index);
    active_pattern       = track.patterns[active_pattern_index];
    active_voice         = &active_pattern->voices[active_voice_index];
    dirty_track          = 1;

    window_gotoxy(&win_Indicators, 0, 0);
    textbuff[0] = 'P';
    itoa_pad(active_pattern_index & 0x0F, &textbuff[1], 16, 'A', '0', 1);
    window_puts(&win_Indicators, textbuff);

    pattern_refresh_steps();
    pattern_color_step(active_step, PATTERN_WINDOW_HL1);
    pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));

    return ERR_SUCCESS;
}

static void pattern_record_note(uint8_t voice_index, uint8_t note)
{
    step_t* step = &active_pattern->voices[voice_index].steps[active_step];
    if (step->note != note) {
        step->note = note;
        dirty_track = 1;
    }
    last_step_edit = step;
    pattern_refresh_step(voice_index, active_step);
    pattern_color_step(active_step, PATTERN_WINDOW_HL1);
    pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
}

static uint8_t pattern_piano_key(uint8_t key)
{
    uint8_t offset = piano_offset(key);
    if (offset == 0xFF)
        return 0;
    if (playing)
        return 1;
    for (uint8_t i = 0; i < NUM_VOICES; i++) {
        if (voice_keys[i] == key)
            return 1;
    }
    for (uint8_t i = 0; i < NUM_VOICES; i++) {
        uint8_t index = (active_voice_index + i) % NUM_VOICES;
        if (voice_keys[index])
            continue;
        uint8_t note = input_octave * 12 + offset;
        voice_keys[index] = key;
        if (recording)
            pattern_record_note(index, note);
        pattern_audition(index, note);
        pattern_show_input_status();
        break;
    }
    /* All four voices busy: ignore extra notes rather than stealing a held voice. */
    return 1;
}

uint8_t pattern_keypress_handler(unsigned char key)
{
    if (pattern_piano_key(key))
        return 1;
    /* Space moves exactly as Down while recording; it never reaches transport. */
    if (key == KB_KEY_SPACE && recording)
        key = KB_DOWN_ARROW;
    switch (key) {
        case KB_KEY_A: {
            if (!recording)
                return 0; // Allow the global Arrangement view shortcut.
            pattern_release_key(voice_keys[active_voice_index]);
            pattern_record_note(active_voice_index, NOTE_OFF);
        } break;
        case KB_KEY_F: {
            if (!recording)
                return 0;
            pattern_release_all();
            for (uint8_t i = 0; i < NUM_VOICES; i++)
                pattern_record_note(i, NOTE_OFF);
        } break;
        case KB_KEY_R: {
            if (!playing) {
                pattern_release_all();
                recording ^= 1;
                pattern_show_input_status();
            }
        } break;
        case KB_KEY_MINUS:
        case KB_KEY_EQUAL: {
            if (key == KB_KEY_MINUS && input_octave > 0)
                input_octave--;
            if (key == KB_KEY_EQUAL && input_octave < (NOTE_OFF / 12) - 1)
                input_octave++;
            pattern_show_input_status();
        } break;
        /** CELL EDIT */
        case KB_LEFT_ARROW: {
            pattern_update_cell(active_voice, -1);
            last_step_edit = &active_voice->steps[active_step];
        } break;
        case KB_RIGHT_ARROW: {
            pattern_update_cell(active_voice, 1);
            last_step_edit = &active_voice->steps[active_step];
        } break;
        case KB_PG_DOWN: {
            pattern_update_cell(active_voice, -2);
            last_step_edit = &active_voice->steps[active_step];
        } break;
        case KB_PG_UP: {
            pattern_update_cell(active_voice, 2);
            last_step_edit = &active_voice->steps[active_step];
        } break;
        case KB_INSERT: {
            if (last_step_edit != NULL) {
                mem_cpy(&active_voice->steps[active_step], last_step_edit, sizeof(step_t));
                pattern_refresh_step(active_voice_index, active_step);
                pattern_color_step(active_step, PATTERN_WINDOW_HL1);
                pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
                dirty_track = 1;
            }
        } break;
        case KB_DELETE: {
            active_voice->steps[active_step].note     = NOTE_OUT_OF_RANGE;
            active_voice->steps[active_step].waveform = WAVEFORM_OUT_OF_RANGE;
            active_voice->steps[active_step].fx1      = FX_OUT_OF_RANGE;
            active_voice->steps[active_step].fx2      = FX_OUT_OF_RANGE;
            pattern_refresh_step(active_voice_index, active_step);
            pattern_color_step(active_step, PATTERN_WINDOW_HL1);
            pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
            dirty_track = 1;
        } break;

        /** CELL NAVIGATION */
        case KB_KEY_TAB: {
            pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, PATTERN_WINDOW_BG));
            active_cell++;
            if (active_cell > 3)
                active_cell = 0;
            pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
        } break;

        /** STEP NAVIGATION */
        case KB_UP_ARROW: {
            pattern_color_step(active_step, PATTERN_WINDOW_FG);
            if (active_step > 0)
                active_step--;
            else
                active_step = (STEPS_PER_PATTERN - 1);
            pattern_color_step(active_step, PATTERN_WINDOW_HL1);
            pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
        } break;
        case KB_DOWN_ARROW: {
            pattern_color_step(active_step, PATTERN_WINDOW_FG);
            active_step++;
            if (active_step > (STEPS_PER_PATTERN - 1))
                active_step = 0;
            pattern_color_step(active_step, PATTERN_WINDOW_HL1);
            pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
        } break;
        case KB_HOME: {
            pattern_color_step(active_step, PATTERN_WINDOW_FG);
            active_step = 0;
            pattern_color_step(active_step, PATTERN_WINDOW_HL1);
            pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
        } break;
        case KB_END: {
            pattern_color_step(active_step, PATTERN_WINDOW_FG);
            active_step = (STEPS_PER_PATTERN - 1);
            pattern_color_step(active_step, PATTERN_WINDOW_HL1);
            pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
        } break;

        /** VOICE NAVIGATION */
        case KB_KEY_1: {
            pattern_color_step(active_step, PATTERN_WINDOW_HL1);
            pattern_select_voice(0);
            pattern_color_step(active_step, PATTERN_WINDOW_HL1);
            pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
        } break;
        case KB_KEY_2: {
            pattern_color_step(active_step, PATTERN_WINDOW_HL1);
            pattern_select_voice(1);
            pattern_color_step(active_step, PATTERN_WINDOW_HL1);
            pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
        } break;
        case KB_KEY_3: {
            pattern_color_step(active_step, PATTERN_WINDOW_HL1);
            pattern_select_voice(2);
            pattern_color_step(active_step, PATTERN_WINDOW_HL1);
            pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
        } break;
        case KB_KEY_4: {
            pattern_color_step(active_step, PATTERN_WINDOW_HL1);
            pattern_select_voice(3);
            pattern_color_step(active_step, PATTERN_WINDOW_HL1);
            pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
        } break;

        /** PATTERN NAVIGATION */
        case KB_KEY_LEFT_BRACKET: {
            pattern_release_all();
            // previous pattern
            active_pattern_index = zmt_pattern_prev(&track);
            active_pattern       = track.patterns[active_pattern_index];
            active_voice         = &active_pattern->voices[active_voice_index];
            pattern_refresh_steps();

            window_gotoxy(&win_Indicators, 0, 0);
            textbuff[0] = 'P';
            itoa_pad(active_pattern_index & 0x0F, &textbuff[1], 16, 'A', '0', 1);
            window_puts(&win_Indicators, textbuff);
        } break;
        case KB_KEY_RIGHT_BRACKET: {
            pattern_release_all();
            // next pattern
            active_pattern_index = zmt_pattern_next(&track);
            active_pattern       = track.patterns[active_pattern_index];
            active_voice         = &active_pattern->voices[active_voice_index];
            pattern_refresh_steps();

            window_gotoxy(&win_Indicators, 0, 0);
            textbuff[0] = 'P';
            itoa_pad(active_pattern_index & 0x0F, &textbuff[1], 16, 'A', '0', 1);
            window_puts(&win_Indicators, textbuff);
        } break;
        case KB_F5: {
            pattern_release_all();
            // new pattern
            if (track.pattern_count >= NUM_PATTERNS) {
                notice_dialog_show("Pattern limit reached");
                break;
            }

            track.pattern_count++;
            active_pattern_index = track.pattern_count - 1;
            active_pattern_index = zmt_pattern_set(&track, active_pattern_index);
            active_pattern       = track.patterns[active_pattern_index];
            active_voice         = &active_pattern->voices[active_voice_index];
            zmt_pattern_init(active_pattern);
            dirty_track = 1;

            window_gotoxy(&win_Indicators, 0, 0);
            textbuff[0] = 'P';
            itoa_pad(active_pattern_index & 0x0F, &textbuff[1], 16, 'A', '0', 1);
            window_puts(&win_Indicators, textbuff);

            pattern_refresh_steps();
            pattern_color_step(active_step, PATTERN_WINDOW_HL1);
            pattern_color_cell(active_step, active_cell, COLOR(PATTERN_WINDOW_HL1, TEXT_COLOR_BLUE));
        } break;
        case KB_F6: {
            // clear pattern
            confirm_handler = &clear_pattern_handler;
            confirm_dialog_show("Clear Pattern?");
        } break;
        case KB_F7: {
            // delete pattern
            if (track.pattern_count < 2)
                break;
            if (pattern_is_referenced(active_pattern_index)) {
                notice_dialog_show("Pattern is used in arrangement");
                break;
            }

            confirm_handler = &delete_pattern_handler;
            confirm_dialog_show("Delete Pattern?");
        } break;
        default: {
            return 0; // unhandled
        }
    }
    return 1;
}
