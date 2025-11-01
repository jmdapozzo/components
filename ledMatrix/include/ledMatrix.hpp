#pragma once

#include <lvgl.h>
#include <esp_lvgl_port.h>

namespace macdap
{
    class LedMatrix
    {

    private:
        lv_display_t *m_display;
        LedMatrix();
        ~LedMatrix();

    public:
        LedMatrix(LedMatrix const&) = delete;
        void operator=(LedMatrix const &) = delete;
        static LedMatrix &get_instance()
        {
            static LedMatrix instance;
            return instance;
        }
        lv_display_t *get_lv_display();
        void set_brightness(float brightness);
    };
}
