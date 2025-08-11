#pragma once

#include <lvgl.h>
#include <esp_lvgl_port.h>

#ifdef __cplusplus
extern "C"
{
#endif

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
        static LedMatrix &getInstance()
        {
            static LedMatrix instance;
            return instance;
        }
        void setBrightness(float brightness);
        lv_display_t *getLvDisplay();
    };
}

#ifdef __cplusplus
}
#endif

