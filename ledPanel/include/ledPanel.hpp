#pragma once

#include "esp_err.h"
#include "driver/spi_master.h"
#include <lvgl.h>
#include <esp_lvgl_port.h>

#ifdef __cplusplus
extern "C"
{
#endif

    namespace macdap
    {
        typedef enum {
            w6h8,           // M12-6x8 (12", 5 led/pixel or 9 led/pixel)
            w12h8,          // M6-12x8 (6", 4 led/pixel)
            w16h8,          // M6-16x8 (6", 3 led/pixel)
            w24h8,          // M4-24x8 (4", 1 led/pixel)
        } matrix_type_t;
    
        class LedPanel
        {

        private:
            lv_disp_draw_buf_t m_displayDrawBuffer;
            lv_disp_drv_t m_displayDriver;
            lv_disp_t *m_lvDisp;
            uint16_t m_horizontalResolution;
            uint16_t m_verticalResolution;
            size_t m_panelBufferSize;
            uint8_t* m_panelBuffer;
#ifdef CONFIG_SPI_LED_PANEL_INTERFACE        
            spi_device_handle_t m_spi;
#endif
            static void Init();

        public:
            LedPanel(matrix_type_t matrixType, uint8_t matrixWidth, uint8_t matrixHeight,int latchGpio = CONFIG_LED_PANEL_LATCH);
            ~LedPanel();
            uint16_t getHorizontalResolution();
            uint16_t getVerticalResolution();
            uint8_t getBuffer(uint16_t index);
            void setBuffer(uint16_t index, uint8_t data);
            void sendBuffer();
            void setBrightness(float brightness);
            lv_disp_t *getLvDisp();
            void seize();
            void release();
            void clear();
            void greeting(const char *projectName, const char *version);
            void message(const char *message);
            void scrollingMessage(const char *message);
        };
    }
#ifdef __cplusplus
}
#endif