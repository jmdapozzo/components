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

#ifndef LV_ATTRIBUTE_IMG_LOGO16X16
#define LV_ATTRIBUTE_IMG_LOGO16X16
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_LOGO16X16 uint8_t logo16x16_map[] = {
  0x00, 0x00, 
  0x03, 0x00, 
  0x06, 0x00, 
  0x04, 0x00, 
  0x18, 0x00, 
  0x18, 0x00, 
  0x0f, 0x00, 
  0x07, 0x8c, 
  0x07, 0x8c, 
  0x06, 0xd8, 
  0x18, 0x70, 
  0x18, 0x70, 
  0x0c, 0xf0, 
  0x03, 0x0c, 
  0x03, 0x8c, 
  0x03, 0x00, 
};

const lv_img_dsc_t logo16x16 = {
  .header.cf = LV_IMG_CF_ALPHA_1BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 16,
  .header.h = 16,
  .data_size = 32,
  .data = logo16x16_map,
};
