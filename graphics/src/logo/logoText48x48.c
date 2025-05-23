#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif


#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_LOGOTEXT48X48
#define LV_ATTRIBUTE_IMG_LOGOTEXT48X48
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_LOGOTEXT48X48 uint8_t logoText48x48_map[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 
  0x00, 0x03, 0xc0, 0x00, 0x00, 0x00, 
  0x00, 0x03, 0xc0, 0x00, 0x00, 0x00, 
  0x00, 0x03, 0xc0, 0x00, 0x00, 0x00, 
  0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1e, 0x00, 0x80, 0x00, 
  0x00, 0x00, 0x1f, 0x03, 0xc0, 0x00, 
  0x00, 0x00, 0x1f, 0x03, 0xc0, 0x00, 
  0x00, 0x00, 0x1f, 0x03, 0xc0, 0x00, 
  0x00, 0x00, 0x1f, 0x07, 0x00, 0x00, 
  0x00, 0x00, 0xe0, 0xfc, 0x00, 0x00, 
  0x00, 0x01, 0xe0, 0x78, 0x00, 0x00, 
  0x00, 0x03, 0xe0, 0xf8, 0x00, 0x00, 
  0x00, 0x03, 0xe0, 0xf8, 0x00, 0x00, 
  0x00, 0x00, 0xe0, 0xf8, 0x00, 0x00, 
  0x00, 0x00, 0x15, 0x87, 0x00, 0x00, 
  0x00, 0x00, 0x1f, 0x03, 0xc0, 0x00, 
  0x00, 0x00, 0x1f, 0x03, 0xc0, 0x00, 
  0x00, 0x00, 0x1f, 0x03, 0xc0, 0x00, 
  0x00, 0x00, 0x0e, 0x01, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x18, 0x70, 0x00, 0x3c, 0x00, 0x00, 
  0x1c, 0x70, 0x00, 0x26, 0x00, 0x00, 
  0x1c, 0xf3, 0x87, 0x27, 0x38, 0xb0, 
  0x14, 0xb3, 0xc7, 0x23, 0x2c, 0xf8, 
  0x14, 0xb0, 0x4c, 0x23, 0x04, 0xc8, 
  0x16, 0xb1, 0xcc, 0x23, 0x3c, 0xc8, 
  0x16, 0xb3, 0x4c, 0x22, 0x6c, 0xc8, 
  0x13, 0x36, 0x4c, 0x22, 0x64, 0xc8, 
  0x13, 0x36, 0xcd, 0x3e, 0x7c, 0xf8, 
  0x13, 0x23, 0x43, 0x38, 0x34, 0xd0, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

const lv_img_dsc_t logoText48x48 = {
  .header.cf = LV_IMG_CF_ALPHA_1BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 48,
  .header.h = 48,
  .data_size = 288,
  .data = logoText48x48_map,
};
