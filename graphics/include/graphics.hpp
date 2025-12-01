#pragma once

#include <esp_err.h>
#include <lvgl.h>
#include <esp_lvgl_port.h>
#include <logos.h>
#include <fonts.h>
#include <icons.h>

namespace macdap
{
    enum class IconSize
    {
        SIZE_16,
        SIZE_32
    };

    enum class WifiStatus
    {
        NONE,
        LOW,
        MEDIUM,
        HIGH,
        DISCONNECTED_SLASH,
        DISCONNECTED_X,
        BROADCAST
    };

    enum class CellularStatus
    {
        NONE,
        LOW,
        MEDIUM,
        HIGH,
        FULL,
        DISCONNECTED_SLASH,
        DISCONNECTED_X
    };

    class Graphics
    {

    private:
        Graphics();
        ~Graphics();

    public:
        Graphics(Graphics const&) = delete;
        void operator=(Graphics const &) = delete;
        static Graphics &get_instance()
        {
            static Graphics instance;
            return instance;
        }

        esp_err_t init(lv_display_t *display);
        bool seize_lvgl(uint32_t ms_timeout = 0);
        void release_lvgl(void);
        void clear(lv_display_t *display);
        void background(lv_display_t *display,lv_color_t color);
        void delete_widget(lv_obj_t *widget);
        lv_obj_t *logo(lv_display_t *display, const void *src, lv_style_t *style = nullptr);
        lv_obj_t *message(lv_display_t *display, const char *message = "", lv_style_t *style = nullptr, lv_label_long_mode_t long_mode = LV_LABEL_LONG_SCROLL_CIRCULAR);
        void qrcode(lv_display_t *display, const char *data, lv_color_t light_color = lv_color_white(), lv_color_t dark_color = lv_color_black());
        void dot(lv_display_t *display, lv_style_t *style = nullptr, int32_t x = 0, int32_t y = 0);
        void horizontal(lv_display_t *display, lv_style_t *style = nullptr);
        void vertical(lv_display_t *display, lv_style_t *style = nullptr);
        void cross(lv_display_t *display, lv_style_t *style = nullptr);
        void spinner(lv_display_t *display, int32_t size, lv_style_t *style = nullptr);
        lv_obj_t *led(lv_display_t *display, int32_t size, lv_color_t color);
        
        // Status icon methods
        lv_obj_t *create_wifi_status_icon(lv_display_t *display, WifiStatus status, IconSize size = IconSize::SIZE_16, int32_t x = 0, int32_t y = 0);
        lv_obj_t *create_cellular_status_icon(lv_display_t *display, CellularStatus status, IconSize size = IconSize::SIZE_16, int32_t x = 0, int32_t y = 0);
        
        // Status icon update methods (for existing icons)
        bool update_wifi_status_icon(lv_obj_t *icon_widget, WifiStatus status, IconSize size = IconSize::SIZE_16);
        bool update_cellular_status_icon(lv_obj_t *icon_widget, CellularStatus status, IconSize size = IconSize::SIZE_16);
        bool update_intensity_status_icon(lv_obj_t *icon_widget, uint16_t intensity, IconSize size = IconSize::SIZE_16);
        
    };
}
