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
            lv_display_t *m_lvDisplay;
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
            lv_display_t *getLvDisplay();
        };
    }
#ifdef __cplusplus
}
#endif