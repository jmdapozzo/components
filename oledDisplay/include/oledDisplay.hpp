#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"

namespace macdap
{
    class Display
    {

    private:
        bool m_is_present;
        lv_display_t *m_lv_display;
        Display();
        ~Display();

    public:
        Display(Display const&) = delete;
        void operator=(Display const &) = delete;
        static Display &get_instance()
        {
            static Display instance;
            return instance;
        }
        bool is_present() const { return m_is_present; }
        lv_display_t *get_lv_display();
    };
}
