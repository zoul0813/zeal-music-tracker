#include <stdint.h>
#include <stddef.h>
#include <zos_errors.h>
#include <zos_vfs.h>
#include <zos_sys.h>

#ifndef COMMON_H
#define COMMON_H

#define KILOBYTE    (uint32_t)(1024)

#define SCREEN_COL80_WIDTH  80
#define SCREEN_COL80_HEIGHT 40
#define SCREEN_COL40_WIDTH  40
#define SCREEN_COL40_HEIGHT 20

#define CH_NULL         '\0'
#define CH_NEWLINE      '\n' // \n
#define CH_SPACE        ' ' // Space
#define CH_TAB          '\t' // \t
#define CH_PERIOD       '.' // .

#define PATH_SEP        '/'
#define DRIVE_SEP       ':'

/** Definitions */
uint16_t put_sn(const char* str, uint16_t size);
uint16_t put_s(const char* str);
uint16_t put_c(const char c);
uint16_t put_u8(uint8_t i);
uint16_t str_len(const char* str);
int16_t str_ends_with(const char *str, const char *suffix);
int16_t str_cmp(void* src1, const void* src2);
int16_t str_pos(const char* str, char c);
void* str_cpy(void* dst, const void* src);
void* str_cat(void* dst, const void* src);
uint16_t max(uint16_t a, uint16_t b);
uint16_t min(uint16_t a, uint16_t b);
void* mem_set(void* ptr, uint8_t value, size_t size);
void* mem_cpy(void* dst, const void* src, size_t size);
void itoa(uint16_t num, char* str, uint16_t base, char alpha);
void itoa_pad(uint16_t num, char* str, uint16_t base, char alpha, char pad, uint8_t size);
void u8tohex(uint8_t value, char* buffer, char alpha);

void put_date(zos_date_t* date, char *buffer);


#endif