#include <oledDisplay.hpp>
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/i2c_master.h>
#include <esp_lcd_panel_io.h>
#include <lvgl.h>
#include <esp_lcd_panel_dev.h>
#include "esp_lcd_panel_vendor.h"
#include <esp_lcd_panel_ssd1306.h>

using namespace macdap;

#define LCD_PIXEL_CLOCK_HZ    (400 * 1000)

#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8

static const char *TAG = "display";

Display::Display()
{
    ESP_LOGI(TAG, "Initializing...");

    m_is_present = false;
    m_lv_display = nullptr;

    i2c_master_bus_handle_t i2c_master_bus_handle;
    ESP_ERROR_CHECK(i2c_master_get_bus_handle(0, &i2c_master_bus_handle));

    if (i2c_master_probe(i2c_master_bus_handle, CONFIG_OLED_DISPLAY_I2C_ADDR, 100) != ESP_OK)
    {
        ESP_LOGW(TAG, "SSD1306 not found at address 0x%02X", CONFIG_OLED_DISPLAY_I2C_ADDR);
        return;
    }

    ESP_LOGI(TAG, "Found SSD1306 at address 0x%02X", CONFIG_OLED_DISPLAY_I2C_ADDR);
    m_is_present = true;

    esp_lcd_panel_io_i2c_config_t i2c_config;
    i2c_config.dev_addr = CONFIG_OLED_DISPLAY_I2C_ADDR;
    i2c_config.scl_speed_hz = LCD_PIXEL_CLOCK_HZ;
    i2c_config.control_phase_bytes = 1;               // According to SSD1306 datasheet
    i2c_config.lcd_cmd_bits = LCD_CMD_BITS;           // According to SSD1306 datasheet
    i2c_config.lcd_param_bits = LCD_CMD_BITS;         // According to SSD1306 datasheet
    i2c_config.dc_bit_offset = 6;                     // According to SSD1306 datasheet

    esp_lcd_panel_io_handle_t io_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_master_bus_handle, &i2c_config, &io_handle));

    esp_lcd_panel_dev_config_t dev_config;
    dev_config.bits_per_pixel = 1;
    dev_config.reset_gpio_num = CONFIG_OLED_DISPLAY_GPIO_RESET;

    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    esp_lcd_panel_ssd1306_config_t ssd1306_config;
    ssd1306_config.height = CONFIG_OLED_DISPLAY_HEIGHT;

    esp_lcd_panel_handle_t panel_handle = NULL;

    dev_config.vendor_config = &ssd1306_config;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &dev_config, &panel_handle));
    
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    const lvgl_port_display_cfg_t port_display_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .control_handle = nullptr,
        .buffer_size = CONFIG_OLED_DISPLAY_WIDTH * CONFIG_OLED_DISPLAY_HEIGHT,
        .double_buffer = true,
        .trans_size = 0,
        .hres = CONFIG_OLED_DISPLAY_WIDTH,
        .vres = CONFIG_OLED_DISPLAY_HEIGHT,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = true,
        },
        .color_format = LV_COLOR_FORMAT_NATIVE,
        .flags = {}
    };

    ESP_LOGI(TAG, "Display resolution: %d x %d", CONFIG_OLED_DISPLAY_WIDTH, CONFIG_OLED_DISPLAY_HEIGHT);

    m_lv_display = lvgl_port_add_disp(&port_display_cfg); // Warning "Callback on_color_trans_done was already set and now it was overwritten!" comes from this call.
}

Display::~Display()
{
}

lv_display_t *Display::get_lv_display()
{
    return m_lv_display;
}
