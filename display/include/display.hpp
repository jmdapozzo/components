#pragma once

#include "esp_err.h"
#include "esp_lvgl_port.h"

#ifdef __cplusplus
extern "C"
{
#endif

    namespace macdap
    {
        class Display
        {

        private:
            lv_disp_t *m_lvDisp;
            Display();
            ~Display();

        public:
            Display(Display const&) = delete;
            void operator=(Display const &) = delete;
            static Display &getInstance()
            {
                static Display instance;
                return instance;
            }

            void clear();
            void greeting(const char *projectName, const char *version);
            void message(const char *message);
            //  esp_err_t showLogoHorizontal(uint8_t chXpos = 0, uint8_t chYpos = 16);
            //  esp_err_t showLogoVertical(uint8_t chXpos = 0, uint8_t chYpos = 0);
            //  esp_err_t showTime(uint8_t chYpos = 16);
        };
    }
#ifdef __cplusplus
}
#endif