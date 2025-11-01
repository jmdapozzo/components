#include <display.hpp>
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/i2c_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <lvgl.h>
#include <esp_lcd_panel_dev.h>
#include <esp_lcd_panel_ssd1306.h>

#if CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
#include "esp_lcd_sh1107.h"
#else
#include "esp_lcd_panel_vendor.h"
#endif

using namespace macdap;

#define LCD_PIXEL_CLOCK_HZ    (400 * 1000)

#if CONFIG_LCD_CONTROLLER_SSD1306
#define DISP_WIDTH              128
#define DISP_HEIGHT             CONFIG_SSD1306_HEIGHT
#elif CONFIG_LCD_CONTROLLER_SH1107
#define DISP_WIDTH              64
#define DISP_HEIGHT             128
#endif

#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8

static const char *TAG = "display";

Display::Display()
{
    ESP_LOGI(TAG, "Initializing...");

    i2c_master_bus_handle_t i2c_master_bus_handle;
    ESP_ERROR_CHECK(i2c_master_get_bus_handle(0, &i2c_master_bus_handle));
    if (i2c_master_probe(i2c_master_bus_handle, CONFIG_I2C_LCD_CONTROLLER_ADDR, 100) != ESP_OK)
    {
        ESP_LOGW(TAG, "I2C device not found");
        return;
    }

    esp_lcd_panel_io_i2c_config_t i2c_config;
    i2c_config.dev_addr = CONFIG_I2C_LCD_CONTROLLER_ADDR;
    i2c_config.scl_speed_hz = LCD_PIXEL_CLOCK_HZ;
    i2c_config.control_phase_bytes = 1;               // According to SSD1306 datasheet
    i2c_config.lcd_cmd_bits = LCD_CMD_BITS;           // According to SSD1306 datasheet
    i2c_config.lcd_param_bits = LCD_CMD_BITS;         // According to SSD1306 datasheet
#if CONFIG_LCD_CONTROLLER_SSD1306
    i2c_config.dc_bit_offset = 6;                     // According to SSD1306 datasheet
#elif CONFIG_LCD_CONTROLLER_SH1107
    i2c_config.dc_bit_offset = 0;                     // According to SH1107 datasheet
    i2c_config.flags =
    {
        .disable_control_phase = 1
    }
#endif

    esp_lcd_panel_io_handle_t io_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_master_bus_handle, &i2c_config, &io_handle));

    esp_lcd_panel_dev_config_t dev_config;
    dev_config.bits_per_pixel = 1;
    dev_config.reset_gpio_num = CONFIG_GPIO_RESET;

#if CONFIG_LCD_CONTROLLER_SSD1306
    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    esp_lcd_panel_ssd1306_config_t ssd1306_config;
    ssd1306_config.height = DISP_HEIGHT;

    esp_lcd_panel_handle_t panel_handle = NULL;

    dev_config.vendor_config = &ssd1306_config;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &dev_config, &panel_handle));
#elif CONFIG_LCD_CONTROLLER_SH1107
    ESP_LOGI(TAG, "Install SH1107 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_sh1107(io_handle, &dev_config, &panel_handle));
#endif

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

#if CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
#endif

    const lvgl_port_display_cfg_t port_display_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = DISP_WIDTH * DISP_HEIGHT,
        .double_buffer = true,
        .hres = DISP_WIDTH,
        .vres = DISP_HEIGHT,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = true,
        }
    };

    ESP_LOGI(TAG, "Display resolution: %d x %d", DISP_WIDTH, DISP_HEIGHT);

    m_lv_display = lvgl_port_add_disp(&port_display_cfg); // Warning "Callback on_color_trans_done was already set and now it was overwritten!" comes from this call.
}

Display::~Display()
{
}

lv_display_t *Display::get_lv_display()
{
    return m_lv_display;
}

