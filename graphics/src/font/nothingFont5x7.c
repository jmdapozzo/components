/*******************************************************************************
 * Size: 7 px
 * Bpp: 1
 * Opts: --bpp 1 --size 7 --no-compress --font nothing-font-5x7.ttf --range 32-127 --format lvgl -o nothingFont5x7.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef NOTHINGFONT5X7
#define NOTHINGFONT5X7 1
#endif

#if NOTHINGFONT5X7

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xfc,

    /* U+0022 "\"" */
    0xfc,

    /* U+0023 "#" */
    0x66, 0xf6, 0xf6,

    /* U+0024 "$" */
    0x6d, 0xc6, 0xd6, 0x40,

    /* U+0025 "%" */
    0x45, 0x52, 0xa2, 0xa2, 0xa8, 0x80,

    /* U+0026 "&" */
    0x45, 0x28, 0x8b, 0xb4,

    /* U+0027 "'" */
    0xe0,

    /* U+0028 "(" */
    0x6a, 0x90,

    /* U+0029 ")" */
    0xd5, 0x70,

    /* U+002A "*" */
    0x74,

    /* U+002B "+" */
    0x44, 0xf4, 0x40,

    /* U+002C "," */
    0x60,

    /* U+002D "-" */
    0xc0,

    /* U+002E "." */
    0xf0,

    /* U+002F "/" */
    0x12, 0x44, 0x80,

    /* U+0030 "0" */
    0x69, 0x99, 0x96,

    /* U+0031 "1" */
    0x75, 0x70,

    /* U+0032 "2" */
    0x69, 0x12, 0x4f,

    /* U+0033 "3" */
    0x69, 0x12, 0x96,

    /* U+0034 "4" */
    0x22, 0x6a, 0xf2,

    /* U+0035 "5" */
    0xf8, 0x8e, 0x96,

    /* U+0036 "6" */
    0x34, 0x8e, 0x96,

    /* U+0037 "7" */
    0xf1, 0x24, 0x44,

    /* U+0038 "8" */
    0x69, 0x96, 0x96,

    /* U+0039 "9" */
    0x69, 0x97, 0x34,

    /* U+003A ":" */
    0xf0,

    /* U+003B ";" */
    0x55, 0x80,

    /* U+003C "<" */
    0x59, 0x40,

    /* U+003D "=" */
    0xf0, 0xf0,

    /* U+003E ">" */
    0x95, 0x80,

    /* U+003F "?" */
    0x69, 0x12, 0x44,

    /* U+0040 "@" */
    0x69, 0xdb, 0x86,

    /* U+0041 "A" */
    0x46, 0x99, 0xf9,

    /* U+0042 "B" */
    0xe9, 0x9e, 0x9e,

    /* U+0043 "C" */
    0x69, 0x88, 0x96,

    /* U+0044 "D" */
    0xe9, 0x99, 0x9e,

    /* U+0045 "E" */
    0xf8, 0x8e, 0x8f,

    /* U+0046 "F" */
    0xf8, 0x8e, 0x88,

    /* U+0047 "G" */
    0x69, 0x8b, 0x96,

    /* U+0048 "H" */
    0x99, 0x9f, 0x99,

    /* U+0049 "I" */
    0xd5, 0x70,

    /* U+004A "J" */
    0x32, 0x22, 0xa4,

    /* U+004B "K" */
    0x9a, 0xcc, 0xa9,

    /* U+004C "L" */
    0x88, 0x88, 0x8f,

    /* U+004D "M" */
    0x9f, 0xdd, 0x99,

    /* U+004E "N" */
    0x99, 0xdd, 0xb9,

    /* U+004F "O" */
    0x69, 0x99, 0x96,

    /* U+0050 "P" */
    0xe9, 0x9e, 0x88,

    /* U+0051 "Q" */
    0x69, 0x99, 0xb5,

    /* U+0052 "R" */
    0xe9, 0x9e, 0xa9,

    /* U+0053 "S" */
    0x69, 0x86, 0x96,

    /* U+0054 "T" */
    0xf4, 0x44, 0x44,

    /* U+0055 "U" */
    0x99, 0x99, 0x96,

    /* U+0056 "V" */
    0x99, 0x99, 0xf4,

    /* U+0057 "W" */
    0x99, 0x9d, 0xd6,

    /* U+0058 "X" */
    0x99, 0x64, 0xf9,

    /* U+0059 "Y" */
    0x99, 0x64, 0x44,

    /* U+005A "Z" */
    0xf1, 0x24, 0xcf,

    /* U+005B "[" */
    0xea, 0xb0,

    /* U+005C "\\" */
    0x84, 0x42, 0x10,

    /* U+005D "]" */
    0xd5, 0x70,

    /* U+005E "^" */
    0x69,

    /* U+005F "_" */
    0xf0,

    /* U+0060 "`" */
    0x90,

    /* U+0061 "a" */
    0xc7, 0xb0,

    /* U+0062 "b" */
    0x93, 0x5b, 0x80,

    /* U+0063 "c" */
    0x72, 0x30,

    /* U+0064 "d" */
    0x25, 0xda, 0x80,

    /* U+0065 "e" */
    0x57, 0xb0,

    /* U+0066 "f" */
    0x2b, 0xa4, 0x80,

    /* U+0067 "g" */
    0x76, 0xf3, 0x80,

    /* U+0068 "h" */
    0x93, 0x7b, 0x40,

    /* U+0069 "i" */
    0x4d, 0x50,

    /* U+006A "j" */
    0x4d, 0x56,

    /* U+006B "k" */
    0x92, 0xed, 0x40,

    /* U+006C "l" */
    0xfc,

    /* U+006D "m" */
    0xcf, 0xeb, 0x50,

    /* U+006E "n" */
    0xde, 0xd0,

    /* U+006F "o" */
    0x56, 0xa0,

    /* U+0070 "p" */
    0xd6, 0xf9, 0x0,

    /* U+0071 "q" */
    0x56, 0xf2, 0x40,

    /* U+0072 "r" */
    0xba, 0x40,

    /* U+0073 "s" */
    0x71, 0xe0,

    /* U+0074 "t" */
    0x5d, 0x22,

    /* U+0075 "u" */
    0xb6, 0xb0,

    /* U+0076 "v" */
    0xb7, 0x60,

    /* U+0077 "w" */
    0xdd, 0xde,

    /* U+0078 "x" */
    0x96, 0x69,

    /* U+0079 "y" */
    0xb6, 0xf2, 0x80,

    /* U+007A "z" */
    0xe7, 0x70,

    /* U+007B "{" */
    0x29, 0x44, 0x40,

    /* U+007C "|" */
    0xfe,

    /* U+007D "}" */
    0xc9, 0x15, 0x80,

    /* U+007E "~" */
    0x4d, 0x20
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 25, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 25, .box_w = 1, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2, .adv_w = 37, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 3, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 6, .adv_w = 75, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 10, .adv_w = 124, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 16, .adv_w = 87, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 20, .adv_w = 25, .box_w = 1, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 21, .adv_w = 50, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 23, .adv_w = 50, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 25, .adv_w = 50, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 26, .adv_w = 75, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 29, .adv_w = 37, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 30, .adv_w = 50, .box_w = 2, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 31, .adv_w = 37, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 32, .adv_w = 75, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 35, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 38, .adv_w = 50, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 40, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 43, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 46, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 49, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 52, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 55, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 58, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 61, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 64, .adv_w = 25, .box_w = 1, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 65, .adv_w = 37, .box_w = 2, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 67, .adv_w = 50, .box_w = 2, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 69, .adv_w = 75, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 71, .adv_w = 50, .box_w = 2, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 73, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 76, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 79, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 82, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 85, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 88, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 91, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 94, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 97, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 100, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 103, .adv_w = 50, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 105, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 108, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 111, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 114, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 117, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 120, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 123, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 126, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 129, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 132, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 135, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 138, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 141, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 144, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 147, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 150, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 153, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 156, .adv_w = 50, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 158, .adv_w = 75, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 161, .adv_w = 50, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 163, .adv_w = 75, .box_w = 4, .box_h = 2, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 164, .adv_w = 75, .box_w = 4, .box_h = 1, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 165, .adv_w = 37, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 166, .adv_w = 62, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 168, .adv_w = 62, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 171, .adv_w = 62, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 173, .adv_w = 62, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 176, .adv_w = 62, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 178, .adv_w = 62, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 181, .adv_w = 62, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 184, .adv_w = 62, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 187, .adv_w = 50, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 189, .adv_w = 37, .box_w = 2, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 191, .adv_w = 62, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 194, .adv_w = 25, .box_w = 1, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 195, .adv_w = 100, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 198, .adv_w = 62, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 200, .adv_w = 62, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 202, .adv_w = 62, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 205, .adv_w = 62, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 208, .adv_w = 62, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 210, .adv_w = 62, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 212, .adv_w = 62, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 214, .adv_w = 62, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 216, .adv_w = 62, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 218, .adv_w = 75, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 220, .adv_w = 75, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 222, .adv_w = 62, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 225, .adv_w = 62, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 227, .adv_w = 62, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 230, .adv_w = 25, .box_w = 1, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 231, .adv_w = 62, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 234, .adv_w = 75, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 1}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};

extern const lv_font_t lv_font_unscii_8;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t nothingFont5x7 = {
#else
lv_font_t nothingFont5x7 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 9,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = 1,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = &lv_font_unscii_8,
#endif
    .user_data = NULL,
};



#endif /*#if NOTHINGFONT5X7*/

