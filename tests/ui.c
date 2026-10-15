#include <assert.h>
#include <stdio.h>
#include <string.h>
#define main zmt_application_main
#include "../src/main.c"
#undef main
#include <dialogs.h>
#include "private.h"

uint8_t zvb_peri_text_curs_x,zvb_peri_text_curs_y,zvb_peri_text_curs_time;
uint8_t zvb_peri_text_color,zvb_peri_text_print_char,vid_ctrl_status;
uint8_t zvb_peri_sound_freq_low,zvb_peri_sound_freq_high,zvb_peri_sound_select;
uint8_t zvb_peri_sound_wave,zvb_peri_sound_volume,zvb_peri_sound_master_vol;
uint8_t zvb_peri_sound_hold,zvb_config_dev_idx;
uint8_t zvb_peri_sound_left_channel,zvb_peri_sound_right_channel;
uint8_t MIRROR_TEXT[40][80],MIRROR_COLOR[40][80];
track_t track;
static pattern_t patterns[NUM_PATTERNS];
pattern_t* active_pattern;
uint8_t active_pattern_index;
char textbuff[PATH_MAX];
static int loads,saves,exits,errors;
static zos_err_t file_error;
static kb_keys_t keys[8];
static uint8_t key_position;
static const char* last_message;

void* mem_cpy(void* dst,const void* src,size_t size) {return memcpy(dst,src,size);}
void* str_cpy(void* dst,const void* src) {return strcpy(dst,src);}
void itoa_pad(uint16_t number,char* str,uint16_t base,char alpha,char pad,uint8_t size)
{
    str[size]=0;
    while(size) {uint8_t digit=number%base;str[--size]=digit<10?'0'+digit:alpha+digit-10;number/=base;}
}
uint16_t put_s(const char* text) {return 0;}
uint16_t put_sn(const char* text,uint16_t size) {return 0;}
uint16_t put_c(char c) {return 0;}
void text_map_vram(void) {}
void text_demap_vram(void) {}
void window(window_t* w) {w->_attrs.pos_x=0;w->_attrs.pos_y=0;}
void window_title(window_t* w,const char* title) {w->title=title;}
void window_active(window_t* w,uint8_t active) {}
void window_gotox(window_t* w,uint8_t x) {w->_attrs.pos_x=x;}
void window_gotoy(window_t* w,uint8_t y) {w->_attrs.pos_y=y;}
void window_gotoxy(window_t* w,uint8_t x,uint8_t y) {window_gotox(w,x);window_gotoy(w,y);}
void window_clrscr(window_t* w) {}
void window_save(void) {memcpy(MIRROR_TEXT,SCR_TEXT,sizeof SCR_TEXT);memcpy(MIRROR_COLOR,SCR_COLOR,sizeof SCR_COLOR);}
void window_restore(void) {memcpy(SCR_TEXT,MIRROR_TEXT,sizeof SCR_TEXT);memcpy(SCR_COLOR,MIRROR_COLOR,sizeof SCR_COLOR);}
uint8_t window_putc(window_t* w,char c) {return 1;}
uint8_t window_putc_color(window_t* w,char c,uint8_t color) {return 1;}
uint8_t window_puts(window_t* w,const char* s) {return strlen(s);}
uint8_t window_puts_color(window_t* w,const char* s,uint8_t color) {return strlen(s);}
void window_banner(window_t* w,uint8_t x,uint8_t y,uint8_t centered,const char* s) {}
void dialog_draw(dialog_t* dialog) {last_message=dialog->message;}
uint8_t dialog_handle_key(const dialog_t* dialog,uint8_t key)
{
    for(uint8_t i=0;i<dialog->action_count;i++) if(dialog->actions[i].key==key)return i;
    return DIALOG_RESULT_NONE;
}
void dialog_open(window_t* w) {window_save();window(w);}
int __exit(zos_err_t error) {exits++;return error;}
void handle_error(zos_err_t error,char* message,uint8_t fatal) {if(error){errors++;if(fatal)__exit(error);}}
zos_err_t kb_mode(void* mode) {return ERR_SUCCESS;}
kb_keys_t getkey(void) {assert(key_position<8);return keys[key_position++];}
unsigned char getch(kb_keys_t key) {return 'x';}
zos_err_t zmt_file_load(track_t* t,const char* file) {loads++;return file_error;}
zos_err_t zmt_file_save(track_t* t,const char* file) {saves++;return file_error;}

static void init(void)
{
    memset(&track,0,sizeof track);
    for(uint8_t p=0;p<NUM_PATTERNS;p++)track.patterns[p]=&patterns[p];
    zmt_track_init(&track);zmt_reset(VOL_75);
    pattern_reset_state();arrange_reset_state();
    active_view=VIEW_NONE;previous_view=VIEW_NONE;
    close_handler=&dialog_close;
    loads=saves=exits=errors=0;dirty_track=0;playing=0;file_error=ERR_SUCCESS;
    keys[0]=KB_KEY_X;keys[1]=KB_KEY_ENTER;key_position=0;
    view_switch(VIEW_PATTERN);
}
static void test_load_confirmation(void)
{
    init();dirty_track=1;
    handle_keypress(KB_KEY_L);
    assert(loads==0 && dirty_track==1);
    assert(strcmp(last_message,"Discard changes and load?")==0);
    handle_keypress(KB_KEY_N);
    assert(loads==0 && dirty_track==1 && active_view==VIEW_PATTERN);
    handle_keypress(KB_KEY_L);handle_keypress(KB_KEY_Y);
    assert(loads==1 && dirty_track==0 && active_view==VIEW_PATTERN);
    init();handle_keypress(KB_KEY_L);
    assert(loads==1 && active_view==VIEW_PATTERN);
}
static void test_recoverable_file_errors(void)
{
    init();dirty_track=1;file_error=ERR_NO_SUCH_ENTRY;
    keys[2]=KB_KEY_ENTER;
    handle_keypress(KB_KEY_L);handle_keypress(KB_KEY_Y);
    assert(loads==1 && dirty_track==1 && active_view==VIEW_PATTERN);
    assert(exits==0 && key_position==3);
    init();dirty_track=1;file_error=ERR_READ_ONLY;
    keys[2]=KB_KEY_ENTER;
    handle_keypress(KB_KEY_S);
    assert(saves==1 && dirty_track==1 && active_view==VIEW_PATTERN);
    assert(exits==0 && key_position==3);
}

static void test_help_preserves_playback(void)
{
    init();track.pattern_count=2;
    zmt_pattern_init(&patterns[1]);
    track.arrangement[1]=(arrangement_t){1,FX_OUT_OF_RANGE};
    view_switch(VIEW_ARRANGER);
    handle_keypress(KB_KEY_SPACE);
    assert(playing && playback_arrangement);
    handle_keypress(KB_KEY_H);
    assert(active_view==VIEW_HELP && playing && playback_arrangement);
    zmt_ticks=0;zmt_next_step=31;
    zmt_tick(&track,playback_arrangement);
    assert(zmt_current_arrangement==1 && zmt_current_pattern==1);
    handle_keypress(KB_KEY_ENTER);
    assert(active_view==VIEW_ARRANGER && playing && playback_arrangement);
}

static void test_pattern_preview_selection(void)
{
    init();track.pattern_count=2;
    zmt_pattern_init(&patterns[1]);
    patterns[0].voices[0].steps[0].note=NOTE_MIDDLE_C;
    patterns[1].voices[0].steps[0].note=NOTE_MIDDLE_C+12;
    track.arrangement[1]=(arrangement_t){1,FX_OUT_OF_RANGE};
    view_switch(VIEW_ARRANGER);handle_keypress(KB_KEY_SPACE);
    zmt_ticks=0;zmt_next_step=31;zmt_tick(&track,playback_arrangement);
    assert(zmt_current_pattern==1);
    handle_keypress(KB_KEY_P);
    assert(active_pattern_index==0 && zmt_current_pattern==0 && !playback_arrangement);
    handle_keypress(KB_KEY_SPACE);handle_keypress(KB_KEY_SPACE);
    zmt_tick(&track,playback_arrangement);
    assert(zmt_current_pattern==0);
    assert(((uint16_t)zvb_peri_sound_freq_high<<8 | zvb_peri_sound_freq_low)==NOTES[NOTE_MIDDLE_C]);
}

static void test_help_is_modal(void)
{
    init();dirty_track=1;
    handle_keypress(KB_KEY_H);
    const uint8_t commands[]={KB_KEY_S,KB_KEY_L,KB_KEY_Q,KB_KEY_P,KB_KEY_A,KB_KEY_SPACE,KB_KEY_H};
    for(uint8_t i=0;i<sizeof commands;i++) {
        handle_keypress(commands[i]);
        assert(active_view==VIEW_HELP && previous_view==VIEW_PATTERN);
        assert(loads==0 && saves==0 && exits==0 && !playing && dirty_track);
        assert(keypress_handler==&help_keypress_handler);
    }
    handle_keypress(KB_KEY_ENTER);
    assert(active_view==VIEW_PATTERN && keypress_handler==&pattern_keypress_handler);
    handle_keypress(KB_KEY_S);
    assert(saves==1 && dirty_track==0 && active_view==VIEW_PATTERN);
}

int main(void)
{
    test_load_confirmation();
    test_recoverable_file_errors();
    test_help_preserves_playback();
    test_pattern_preview_selection();
    test_help_is_modal();
    puts("ZMT interface tests passed");
}
