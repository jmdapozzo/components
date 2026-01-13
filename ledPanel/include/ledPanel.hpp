#pragma once

#include "esp_err.h"
#include "driver/spi_master.h"
#include <lvgl.h>
#include <esp_lvgl_port.h>

namespace macdap
{

#ifdef CONFIG_LED_PANEL_TYPE_MAX7219
    typedef struct {
        uint8_t command;
        uint8_t data;
    } max_7219_buffer_t;
#endif

    class LedPanel
    {

    private:
        lv_display_t *m_display;
        size_t m_panel_buffer_size;
        uint8_t* m_panel_buffer;
#ifdef CONFIG_LED_PANEL_TYPE_MAX7219
        size_t m_max_7219_buffer_len;
        max_7219_buffer_t* m_max_7219_buffer;
#endif
#ifdef CONFIG_LED_PANEL_INTERFACE_SPI
        spi_device_handle_t m_spi;
#endif
        LedPanel();
        ~LedPanel();

    public:
        LedPanel(LedPanel const&) = delete;
        void operator=(LedPanel const &) = delete;
        static LedPanel &get_instance()
        {
            static LedPanel instance;
            return instance;
        }
        lv_display_t *get_lv_display();
        uint8_t get_buffer(uint16_t index);
        void set_buffer(uint16_t index, uint8_t data);
        void send_buffer(void *buffer, size_t buffer_size);
        void send_buffer();
        void set_intensity(float intensity);
    };
}
