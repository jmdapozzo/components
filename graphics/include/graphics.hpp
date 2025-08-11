#pragma once

#include <esp_err.h>
#include <lvgl.h>
#include <esp_lvgl_port.h>
#include <logo.h>
#include <font.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include <lvgl.h>

namespace macdap
{
    class Graphics
    {

    private:
        Graphics();
        ~Graphics();

    public:
        Graphics(Graphics const&) = delete;
        void operator=(Graphics const &) = delete;
        static Graphics &getInstance()
        {
            static Graphics instance;
            return instance;
        }

        esp_err_t init(lv_display_t *display);
        void clear(lv_display_t *display);
        void background(lv_display_t *display,lv_color_t color);
        void logo(lv_display_t *display, const void *src);
        void greeting(lv_display_t *display, lv_style_t *style, const char *projectName, const char *version); 
        lv_obj_t * message(lv_display_t *display, lv_style_t *style, const char *message);
        lv_obj_t * scrollingMessageTop(lv_display_t *display, lv_style_t *style, const char *message);
        lv_obj_t * scrollingMessageCenter(lv_display_t *display, lv_style_t *style, const char *message);
        lv_obj_t * scrollingMessageBottom(lv_display_t *display, lv_style_t *style, const char *message);
        void qrcode(lv_display_t *display);
        void cross(lv_display_t *display);
        void spinner(lv_display_t *display);
        lv_obj_t *led(lv_display_t *display);
        
    };
}

#ifdef __cplusplus
}
#endif





