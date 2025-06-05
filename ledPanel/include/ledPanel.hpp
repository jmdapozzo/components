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

#ifdef CONFIG_LED_PANEL_TYPE_MAX7219
    typedef struct {
        uint8_t command;
        uint8_t data;
    } Max7219Buffer_t;
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
        uint8_t* m_panelBuffer;
#ifdef CONFIG_LED_PANEL_TYPE_MAX7219
        size_t m_max7219BufferLen;
        Max7219Buffer_t* m_max7219Buffer;
#endif
#ifdef CONFIG_LED_PANEL_INTERFACE_SPI
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
        uint8_t getBuffer(uint16_t index);
        void setBuffer(uint16_t index, uint8_t data);
        void sendBuffer(void *buffer, size_t bufferSize);
        void sendBuffer();
        void setBrightness(float brightness);
        lv_disp_t *getLvDisp();
        void seize();
        void release();
        void clear();
        void greeting(const char *projectName, const char *version);
        void message(const char *message);
        void scrollingMessage(const char *message);
        void test();
    };
}
#ifdef __cplusplus
}
#endif