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
#ifdef CONFIG_MBI5026_LED_PANEL_TYPE
    typedef struct {
        int8_t data;
    } panelBuffer_t;
#elif CONFIG_MAX7219_LED_PANEL_TYPE
    typedef struct {
        int8_t data;
        int8_t command;
    } panelBuffer_t;
#endif

    class LedPanel
    {

    private:
        lv_disp_draw_buf_t m_displayDrawBuffer;
        lv_disp_drv_t m_displayDriver;
        lv_disp_t *m_lvDisp;
        uint16_t m_horizontalResolution;
        uint16_t m_verticalResolution;
        size_t m_panelBufferSize;
        panelBuffer_t* m_panelBuffer;
#ifdef CONFIG_SPI_LED_PANEL_INTERFACE        
        spi_device_handle_t m_spi;
#endif
        LedPanel();
        ~LedPanel();

    public:
        LedPanel(LedPanel const&) = delete;
        void operator=(LedPanel const &) = delete;
        static LedPanel &getInstance()
        {
            static LedPanel instance;
            return instance;
        }
        uint16_t getHorizontalResolution();
        uint16_t getVerticalResolution();
        panelBuffer_t getBuffer(uint16_t index);
        void setBuffer(uint16_t index, panelBuffer_t buffer);
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