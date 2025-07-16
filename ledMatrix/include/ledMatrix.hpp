#include "esp_err.h"
#include <lvgl.h>
#include <esp_lvgl_port.h>

namespace macdap
{
    class LedMatrix
    {

    private:
        lv_display_t *m_displayDriver;
        lv_disp_t *m_lvDisp;
        uint16_t m_horizontalResolution;
        uint16_t m_verticalResolution;
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
        lv_disp_t *getLvDisp();
    };
}
