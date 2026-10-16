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
static uint8_t keys[256];
static uint16_t key_position, key_count;
static uint8_t read_chunk = 1;
static zos_err_t input_error;
static const char* last_message;
extern uint8_t active_step, active_cell, active_voice_index;

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
zos_err_t ioctl(zos_dev_t dev,uint8_t cmd,void* arg) {return ERR_SUCCESS;}
zos_err_t read(zos_dev_t dev,void* buffer,uint16_t* size)
{
    if(input_error){*size=0;return input_error;}
    uint16_t count=key_count-key_position;
    uint8_t chunk=count && keys[key_position]==KB_RELEASED ? 2 : read_chunk;
    if(count>chunk)count=chunk;
    if(count>*size)count=*size;
    if(count && key_position+count<key_count && keys[key_position+count-1]==KB_RELEASED)count--;
    memcpy(buffer,keys+key_position,count);key_position+=count;*size=count;
    return ERR_SUCCESS;
}
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
    keys[0]=KB_KEY_X;keys[1]=KB_KEY_ENTER;key_position=0;key_count=2;read_chunk=1;input_error=0;
    assert(input_init()==ERR_SUCCESS);
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
    keys[2]=KB_KEY_ENTER;key_count=3;
    handle_keypress(KB_KEY_L);handle_keypress(KB_KEY_Y);
    assert(loads==1 && dirty_track==1 && active_view==VIEW_PATTERN);
    assert(exits==0 && key_position==3);
    init();dirty_track=1;file_error=ERR_READ_ONLY;
    keys[2]=KB_KEY_ENTER;key_count=3;
    handle_keypress(KB_F2);
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
    handle_keypress(KB_F1);
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
    handle_keypress(KB_F1);
    const uint8_t commands[]={KB_F2,KB_KEY_L,KB_KEY_Q,KB_KEY_P,KB_KEY_A,KB_KEY_SPACE,KB_F1,KB_KEY_Z,KB_KEY_R};
    for(uint8_t i=0;i<sizeof commands;i++) {
        handle_keypress(commands[i]);
        assert(active_view==VIEW_HELP && previous_view==VIEW_PATTERN);
        assert(loads==0 && saves==0 && exits==0 && !playing && dirty_track);
        assert(keypress_handler==&help_keypress_handler);
    }
    handle_keypress(KB_KEY_ENTER);
    assert(active_view==VIEW_PATTERN && keypress_handler==&pattern_keypress_handler);
    handle_keypress(KB_F2);
    assert(saves==1 && dirty_track==0 && active_view==VIEW_PATTERN);
}

static void queue_input(const uint8_t* bytes,uint16_t size)
{
    assert(size<=sizeof keys);
    memcpy(keys,bytes,size);key_position=0;key_count=size;read_chunk=32;
}

static void pump_input(void)
{
    input_event_t event;
    for(uint16_t i=0;i<512;i++) {
        assert(input_next(&event)==ERR_SUCCESS);
        if(!event.key)return;
        handle_keyevent(&event);
    }
    assert(!"Input did not drain");
}

static void test_raw_chords_and_recording(void)
{
    init();
    pattern_t original=patterns[0];
    const uint8_t chord[]={KB_KEY_Z,KB_KEY_C,KB_KEY_B};
    queue_input(chord,sizeof chord);pump_input();
    assert(!dirty_track && memcmp(&original,&patterns[0],sizeof original)==0);
    assert(zvb_peri_sound_select==VOICE2 && zvb_peri_sound_volume==VOL_75);
    assert(((uint16_t)zvb_peri_sound_freq_high<<8|zvb_peri_sound_freq_low)==NOTES[NOTE_MIDDLE_C+7]);
    const uint8_t release[]={KB_RELEASED,KB_KEY_C};
    queue_input(release,sizeof release);pump_input();
    assert(zvb_peri_sound_select==VOICE1 && zvb_peri_sound_volume==VOL_0);

    /* Arm recording; the three key presses in one OS read must all survive. */
    const uint8_t record_chord[]={KB_KEY_R,KB_RELEASED,KB_KEY_Z,KB_RELEASED,KB_KEY_B,
        KB_KEY_Z,KB_KEY_C,KB_KEY_B,KB_KEY_SPACE};
    queue_input(record_chord,sizeof record_chord);pump_input();
    assert(dirty_track && !playing && active_step==1);
    assert(patterns[0].voices[0].steps[0].note==NOTE_MIDDLE_C);
    assert(patterns[0].voices[1].steps[0].note==NOTE_MIDDLE_C+4);
    assert(patterns[0].voices[2].steps[0].note==NOTE_MIDDLE_C+7);
    assert(patterns[0].voices[3].steps[0].note==NOTE_OUT_OF_RANGE);
    for(uint8_t v=0;v<NUM_VOICES;v++) {
        assert(patterns[0].voices[v].steps[1].note==NOTE_OUT_OF_RANGE);
        original.voices[v].steps[0].note=patterns[0].voices[v].steps[0].note;
    }
    assert(memcmp(&original,&patterns[0],sizeof original)==0);
    /* Typematic repeats do not add notes, toggle modes, or advance more rows. */
    const uint8_t repeats[]={KB_KEY_Z,KB_KEY_C,KB_KEY_B,KB_KEY_R,KB_KEY_SPACE};
    queue_input(repeats,sizeof repeats);pump_input();
    assert(active_step==1 && !playing && memcmp(&original,&patterns[0],sizeof original)==0);
}

static void test_record_voice_and_cell_editing(void)
{
    init();handle_keypress(KB_KEY_R);handle_keypress(KB_KEY_3);
    assert(active_voice_index==2);
    handle_keypress(KB_KEY_TAB);handle_keypress(KB_RIGHT_ARROW);
    handle_keypress(KB_KEY_TAB);handle_keypress(KB_RIGHT_ARROW);
    handle_keypress(KB_KEY_TAB);handle_keypress(KB_RIGHT_ARROW);
    step_t configured=patterns[0].voices[2].steps[0];
    assert(active_cell==Cell_Effect2 && configured.waveform==0);
    assert(configured.fx1==0 && configured.fx2==0);
    handle_keypress(KB_KEY_Z);configured.note=NOTE_MIDDLE_C;
    assert(memcmp(&configured,&patterns[0].voices[2].steps[0],sizeof configured)==0);
    handle_keypress(KB_KEY_C);handle_keypress(KB_KEY_B);
    assert(patterns[0].voices[3].steps[0].note==NOTE_MIDDLE_C+4);
    assert(patterns[0].voices[0].steps[0].note==NOTE_MIDDLE_C+7);
    /* Voice changes during a chord do not redirect key releases. */
    handle_keypress(KB_KEY_2);pattern_release_key(KB_KEY_Z);
    assert(zvb_peri_sound_select==VOICE2 && zvb_peri_sound_volume==VOL_0);
    handle_keypress(KB_KEY_S);
    assert(patterns[0].voices[1].steps[0].note==NOTE_MIDDLE_C+1);
    handle_keypress(KB_KEY_D); /* Fourth held note uses the remaining voice. */
    assert(patterns[0].voices[2].steps[0].note==NOTE_MIDDLE_C+3);
    pattern_t full=patterns[0];
    handle_keypress(KB_KEY_J); /* A fifth held note must not steal a voice. */
    assert(memcmp(&full,&patterns[0],sizeof full)==0);
}

static void test_record_note_off(void)
{
    init();handle_keypress(KB_KEY_R);handle_keypress(KB_KEY_3);
    handle_keypress(KB_KEY_TAB);handle_keypress(KB_RIGHT_ARROW);
    handle_keypress(KB_KEY_TAB);handle_keypress(KB_RIGHT_ARROW);
    handle_keypress(KB_KEY_TAB);handle_keypress(KB_RIGHT_ARROW);
    handle_keypress(KB_KEY_Z);handle_keypress(KB_KEY_C);
    step_t expected=patterns[0].voices[2].steps[0];
    expected.note=NOTE_OFF;
    pattern_t original=patterns[0];
    original.voices[2].steps[0]=expected;
    dirty_track=0;
    const uint8_t off[]={KB_KEY_A};
    queue_input(off,sizeof off);pump_input();
    assert(active_view==VIEW_PATTERN && active_step==0 && active_voice_index==2);
    assert(active_cell==Cell_Effect2 && dirty_track && !playing);
    assert(memcmp(&original,&patterns[0],sizeof original)==0);
    assert(zvb_peri_sound_select==VOICE2 && zvb_peri_sound_volume==VOL_0);
    /* OFF frees only the selected voice; the other held chord note survives. */
    handle_keypress(KB_KEY_S);
    assert(patterns[0].voices[2].steps[0].note==NOTE_MIDDLE_C+1);
    assert(patterns[0].voices[3].steps[0].note==NOTE_MIDDLE_C+4);
    pattern_release_key(KB_KEY_S);
    handle_keypress(KB_KEY_A);
    dirty_track=0;handle_keypress(KB_KEY_A);
    assert(!dirty_track); /* Replacing OFF with OFF leaves the track clean. */
    handle_keypress(KB_KEY_SPACE);
    /* A held A must not write another OFF after advancing. */
    queue_input(off,sizeof off);pump_input();
    assert(patterns[0].voices[2].steps[1].note==NOTE_OUT_OF_RANGE);
    const uint8_t release[]={KB_RELEASED,KB_KEY_A};
    queue_input(release,sizeof release);pump_input();
    assert(patterns[0].voices[2].steps[1].note==NOTE_OUT_OF_RANGE);
    handle_keypress(KB_INSERT);
    assert(memcmp(&expected,&patterns[0].voices[2].steps[1],sizeof expected)==0);
    handle_keypress(KB_KEY_R);handle_keypress(KB_KEY_A);
    assert(active_view==VIEW_ARRANGER);
}

static void test_record_all_off(void)
{
    init();
    pattern_t original=patterns[0];
    handle_keypress(KB_KEY_F);
    assert(!dirty_track && memcmp(&original,&patterns[0],sizeof original)==0);
    handle_keypress(KB_KEY_SPACE);handle_keypress(KB_KEY_F);
    assert(playing && !dirty_track && memcmp(&original,&patterns[0],sizeof original)==0);
    handle_keypress(KB_KEY_SPACE);handle_keypress(KB_KEY_R);
    handle_keypress(KB_KEY_3);handle_keypress(KB_KEY_TAB);
    for(uint8_t v=0;v<NUM_VOICES;v++) {
        step_t* step=&patterns[0].voices[v].steps[0];
        step->waveform=v;step->fx1=FX_VOL_25_0;step->fx2=FX_GOTO_31;
    }
    handle_keypress(KB_KEY_Z);handle_keypress(KB_KEY_C);
    handle_keypress(KB_KEY_B);handle_keypress(KB_KEY_N);
    original=patterns[0];
    for(uint8_t v=0;v<NUM_VOICES;v++)original.voices[v].steps[0].note=NOTE_OFF;
    dirty_track=0;
    const uint8_t off[]={KB_KEY_F};
    queue_input(off,sizeof off);pump_input();
    assert(dirty_track && !playing && active_view==VIEW_PATTERN);
    assert(active_step==0 && active_voice_index==2 && active_cell==Cell_Waveform);
    assert(memcmp(&original,&patterns[0],sizeof original)==0);
    assert(zvb_peri_sound_volume==VOL_0);
    dirty_track=0;handle_keypress(KB_KEY_F);assert(!dirty_track);
    handle_keypress(KB_KEY_SPACE);
    queue_input(off,sizeof off);pump_input(); /* A held F cannot fill later rows. */
    assert(memcmp(&original,&patterns[0],sizeof original)==0);
    const uint8_t release[]={KB_RELEASED,KB_KEY_F};
    queue_input(release,sizeof release);pump_input();
    assert(memcmp(&original,&patterns[0],sizeof original)==0);
    /* All held piano keys were released, so each voice is available again. */
    const char* chord="zcbn";
    for(uint8_t i=0;i<NUM_VOICES;i++)handle_keypress(chord[i]);
    for(uint8_t v=0;v<NUM_VOICES;v++)
        assert(patterns[0].voices[v].steps[1].note<NOTE_OFF);
    queue_input(off,sizeof off);pump_input();
    for(uint8_t v=0;v<NUM_VOICES;v++)
        assert(patterns[0].voices[v].steps[1].note==NOTE_OFF);
}

static void test_octaves_and_note_mapping(void)
{
    init();handle_keypress(KB_KEY_R);
    const char* piano="zsxdcvgbhnjm";
    for(uint8_t i=0;i<12;i++) {
        handle_keypress(piano[i]);
        assert(patterns[0].voices[0].steps[0].note==NOTE_MIDDLE_C+i);
        pattern_release_key(piano[i]);
    }
    for(uint8_t i=0;i<12;i++)handle_keypress(KB_KEY_MINUS);
    handle_keypress(KB_KEY_Z);
    assert(patterns[0].voices[0].steps[0].note==0);
    /* Octave changes while held never change the voice's release owner. */
    for(uint8_t i=0;i<12;i++)handle_keypress(KB_KEY_EQUAL);
    pattern_release_key(KB_KEY_Z);
    assert(zvb_peri_sound_volume==VOL_0);
    handle_keypress(KB_KEY_M);
    assert(patterns[0].voices[0].steps[0].note==NOTE_OFF-1);
    handle_keypress(KB_END);handle_keypress(KB_KEY_SPACE);
    assert(active_step==0 && !playing);
}

static void test_audition_inherits_sound_without_running_fx(void)
{
    init();
    step_t* early=&patterns[0].voices[0].steps[0];
    early->waveform=WAV_TRIANGLE;early->fx1=FX_VOL_25_0;
    early->fx2=FX_GOTO_31;
    handle_keypress(KB_DOWN_ARROW);
    patterns[0].voices[0].steps[1].fx1=FX_COUNT_3;
    pattern_t original=patterns[0];
    uint8_t next=zmt_next_step;
    handle_keypress(KB_KEY_Z);
    assert(!dirty_track && zmt_next_step==next);
    assert(memcmp(&original,&patterns[0],sizeof original)==0);
    assert(zvb_peri_sound_wave==(WAV_TRIANGLE|DUTY_CYCLE_50_0));
    assert(zvb_peri_sound_volume==VOL_25);
    pattern_release_key(KB_KEY_Z);
    /* A waveform effect on the current row wins over earlier settings. */
    patterns[0].voices[0].steps[1].fx2=FX_VOICE_NOISE;
    handle_keypress(KB_KEY_Z);
    assert(zvb_peri_sound_wave==(WAV_NOISE|DUTY_CYCLE_50_0));
    pattern_release_key(KB_KEY_Z);
    assert(zvb_peri_sound_volume==VOL_0);
}

static void test_modes_and_modal_cleanup(void)
{
    init();handle_keypress(KB_KEY_SPACE);handle_keypress(KB_KEY_R);
    handle_keypress(KB_KEY_Z);
    assert(playing && !dirty_track);
    handle_keypress(KB_KEY_SPACE);assert(!playing);
    handle_keypress(KB_KEY_R);handle_keypress(KB_KEY_Z);
    handle_keypress(KB_F1);
    assert(active_view==VIEW_HELP && zvb_peri_sound_volume==VOL_0);
    handle_keypress(KB_KEY_ENTER);handle_keypress(KB_KEY_SPACE);
    assert(active_step==1 && !playing); /* Recording survives help. */
    handle_keypress(KB_KEY_Z);handle_keypress(KB_F6);
    assert(zvb_peri_sound_volume==VOL_0);
    pattern_t original=patterns[0];
    handle_keypress(KB_KEY_C);handle_keypress(KB_KEY_R);
    assert(memcmp(&original,&patterns[0],sizeof original)==0);
    handle_keypress(KB_KEY_N); /* Cancel clear. */
    view_switch(VIEW_ARRANGER);handle_keypress(KB_KEY_P);
    handle_keypress(KB_KEY_SPACE);
    assert(playing); /* Leaving Pattern view disarms recording. */
}

static void test_shared_dialog_input_and_text(void)
{
    init();
    /* Commands, dialog text and trailing events share a single buffered read. */
    const uint8_t filename[]={KB_F2,KB_LEFT_SHIFT,KB_KEY_Z,KB_RELEASED,KB_KEY_Z,
        KB_RELEASED,KB_LEFT_SHIFT,KB_KEY_SPACE,KB_KEY_X,KB_KEY_ENTER,KB_KEY_R};
    queue_input(filename,sizeof filename);pump_input();
    assert(saves==1 && strcmp(textbuff,"Z x")==0);
    handle_keypress(KB_KEY_SPACE);assert(!playing && active_step==1);
    assert(input_init()==ERR_SUCCESS);
    const uint8_t modifiers[]={KB_LEFT_SHIFT,KB_RIGHT_SHIFT,KB_RELEASED,KB_LEFT_SHIFT,
        KB_KEY_1,KB_RELEASED,KB_RIGHT_SHIFT,KB_CAPS_LOCK,KB_KEY_A,
        KB_LEFT_SHIFT,KB_KEY_B};
    queue_input(modifiers,sizeof modifiers);
    assert(input_getkey()==0);assert(input_getkey()==0);assert(input_getkey()==0);
    assert(input_getkey()==KB_KEY_1 && input_getch(KB_KEY_1)=='!');
    assert(input_getkey()==0);assert(input_getkey()==0);
    assert(input_getkey()==KB_KEY_A && input_getch(KB_KEY_A)=='A');
    assert(input_getkey()==0);
    assert(input_getkey()==KB_KEY_B && input_getch(KB_KEY_B)=='b');
    input_event_t event;
    input_error=ERR_FAILURE;
    assert(input_next(&event)==ERR_FAILURE && event.key==0);
    input_error=0;
}

static void test_raw_input_across_reads(void)
{
    init();key_position=key_count=0;read_chunk=32;
    /* Many press/release pairs cross the 32-byte read boundary. */
    for(uint8_t i=0;i<16;i++) {
        keys[key_count++]=KB_KEY_Z;
        keys[key_count++]=KB_RELEASED;
        keys[key_count++]=KB_KEY_Z;
    }
    keys[key_count++]=KB_KEY_Z;
    keys[key_count++]=KB_KEY_C;
    keys[key_count++]=KB_KEY_B;
    pump_input();
    assert(zvb_peri_sound_select==VOICE2 && zvb_peri_sound_volume==VOL_75);
    assert(!dirty_track);
    const uint8_t all_up[]={KB_RELEASED,KB_KEY_Z,KB_RELEASED,KB_KEY_C,KB_RELEASED,KB_KEY_B};
    queue_input(all_up,sizeof all_up);pump_input();
    assert(zvb_peri_sound_volume==VOL_0);
    /* All voices are available again after releasing the chord. */
    handle_keypress(KB_KEY_R);
    const uint8_t four[]={KB_KEY_Z,KB_KEY_C,KB_KEY_B,KB_KEY_M};
    queue_input(four,sizeof four);pump_input();
    for(uint8_t v=0;v<NUM_VOICES;v++)assert(patterns[0].voices[v].steps[0].note!=NOTE_OUT_OF_RANGE);
}

static void test_remapped_pattern_commands(void)
{
    init();handle_keypress(KB_KEY_R);handle_keypress(KB_F5);
    assert(track.pattern_count==2 && active_pattern_index==1);
    handle_keypress(KB_KEY_N);
    assert(track.pattern_count==2 && patterns[1].voices[0].steps[0].note==NOTE_MIDDLE_C+9);
    handle_keypress(KB_F6);handle_keypress(KB_KEY_Y);
    assert(patterns[1].voices[0].steps[0].note==NOTE_OUT_OF_RANGE);
    handle_keypress(KB_F7);handle_keypress(KB_KEY_Y);
    assert(track.pattern_count==1 && active_pattern_index==0);
    handle_keypress(KB_KEY_SPACE);
    assert(!playing && active_step==1);
}

int main(void)
{
    test_load_confirmation();
    test_recoverable_file_errors();
    test_help_preserves_playback();
    test_pattern_preview_selection();
    test_help_is_modal();
    test_raw_chords_and_recording();
    test_record_voice_and_cell_editing();
    test_record_note_off();
    test_record_all_off();
    test_octaves_and_note_mapping();
    test_audition_inherits_sound_without_running_fx();
    test_modes_and_modal_cleanup();
    test_shared_dialog_input_and_text();
    test_raw_input_across_reads();
    test_remapped_pattern_commands();
    puts("ZMT interface tests passed");
}
