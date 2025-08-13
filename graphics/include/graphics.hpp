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
        bool seizeLvgl(uint32_t msTimeout = 0);
        void releaseLvgl(void);
        void clear(lv_display_t *display);
        void background(lv_display_t *display,lv_color_t color);
        void logo(lv_display_t *display, const void *src);
        lv_obj_t * message(lv_display_t *display, lv_style_t *style = nullptr, const char *message = "", lv_label_long_mode_t longMode = LV_LABEL_LONG_SCROLL_CIRCULAR);
        void qrcode(lv_display_t *display, const char *data, lv_color_t lightColor = lv_color_white(), lv_color_t darkColor = lv_color_black());
        void cross(lv_display_t *display, lv_style_t *style = nullptr);
        void spinner(lv_display_t *display, int32_t size, lv_style_t *style = nullptr);
        lv_obj_t *led(lv_display_t *display, int32_t size, lv_color_t color);
        
    };
}

#ifdef __cplusplus
}
#endif





