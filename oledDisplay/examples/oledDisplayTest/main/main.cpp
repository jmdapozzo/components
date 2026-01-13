#include <oledDisplay.hpp>
#include <graphics.hpp>
#include <driver/i2c_master.h>
#include <string.h>
#include <esp_log.h>
#include <logos.h>

static const char *TAG = "displayTest";

static lv_anim_t _animation;
static lv_style_t _style_base;
static lv_style_t _style_greeting_top;
static lv_style_t _style_greeting_bottom;
static lv_style_t _style_main_message;
static lv_style_t _style_main_message_top;
static lv_style_t _style_main_message_bottom;

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "LED Matrix Test Program");

    i2c_master_bus_config_t i2c_master_bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = static_cast<gpio_num_t>(CONFIG_GPIO_I2C_SDA),
        .scl_io_num = static_cast<gpio_num_t>(CONFIG_GPIO_I2C_SCL),
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7
    };
    i2c_master_bus_config.flags.enable_internal_pullup = true;

    i2c_master_bus_handle_t i2c_master_bus_handle = NULL;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_master_bus_config, &i2c_master_bus_handle));

    ESP_LOGI(TAG, "Probing...");
    for (int i = 0; i < 0x80; i++)
    {
        if (i2c_master_probe(i2c_master_bus_handle, i, 100) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found device at 0x%02x", i);
        }
    }
    ESP_LOGI(TAG, "Done probing!");

    macdap::Graphics &graphics = macdap::Graphics::get_instance();

    macdap::Display &display = macdap::Display::get_instance();
    lv_disp_t *lv_display = display.get_lv_display();
    graphics.init(lv_display);

    int32_t width = lv_display_get_horizontal_resolution(lv_display);
    int32_t height = lv_display_get_vertical_resolution(lv_display);

    lv_anim_init(&_animation);
    lv_anim_set_delay(&_animation, 1000);
    lv_anim_set_repeat_delay(&_animation, 3000);
    lv_anim_set_repeat_count(&_animation, LV_ANIM_REPEAT_INFINITE);

    lv_style_init(&_style_base);
    lv_style_set_text_align(&_style_base, LV_TEXT_ALIGN_CENTER);
    lv_style_set_align(&_style_base, LV_ALIGN_CENTER);
    lv_style_set_text_color(&_style_base, lv_color_black());
    lv_style_set_width(&_style_base, width);
    lv_style_set_line_width(&_style_base, 3);
    lv_style_set_line_color(&_style_base, lv_color_black());
    lv_style_set_line_rounded(&_style_base, true);
    lv_style_set_anim(&_style_base, &_animation);

    lv_style_copy(&_style_greeting_top, &_style_base);
    lv_style_set_text_font(&_style_greeting_top, &lv_font_montserrat_14);
    lv_style_set_align(&_style_greeting_top, LV_ALIGN_TOP_LEFT);

    lv_style_copy(&_style_greeting_bottom, &_style_base);
    lv_style_set_text_font(&_style_greeting_bottom, &lv_font_montserrat_14);
    lv_style_set_align(&_style_greeting_bottom, LV_ALIGN_BOTTOM_LEFT);

    lv_style_copy(&_style_main_message, &_style_base);
    lv_style_set_text_font(&_style_main_message, &lv_font_montserrat_24);

    lv_style_copy(&_style_main_message_top, &_style_main_message);
    lv_style_set_align(&_style_main_message_top, LV_ALIGN_TOP_LEFT);

    lv_style_copy(&_style_main_message_bottom, &_style_main_message);
    lv_style_set_align(&_style_main_message_bottom, LV_ALIGN_BOTTOM_LEFT);

    graphics.background(lv_display, lv_color_white());

    graphics.logo(lv_display, &colorLogoNoText64x64);
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lv_display);

    graphics.create_message(lv_display, "Test", &_style_greeting_top);
    graphics.create_message(lv_display, "Version 0.0.0", &_style_greeting_bottom);
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lv_display);

    graphics.create_message(lv_display, "MacDap Inc.", &_style_main_message);
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lv_display);

    graphics.qrcode(lv_display, "https://macdap.com");
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lv_display);

    graphics.cross(lv_display, &_style_base);
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lv_display);

    graphics.create_message(lv_display, "This is a MacDap application!", &_style_main_message);
    vTaskDelay(pdMS_TO_TICKS(5000));
    graphics.clear(lv_display);

    graphics.spinner(lv_display, height/2, &_style_base);
    vTaskDelay(pdMS_TO_TICKS(5000));
    graphics.clear(lv_display);

    // TODO Toggling does not work
    // lv_obj_t *heartbeat = graphics.led(lv_display, lv_color_black());

    graphics.create_message(lv_display, "Test program Version 0.0.0 from MacDap Inc.", &_style_main_message_top);
    graphics.create_message(lv_display, "MacDap Inc. the best", &_style_main_message_bottom);

    bool phase = false;
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        if (phase)
        {
            phase = false;
            // lv_led_on(heartbeat);
        }
        else
        {
            phase = true;
            // lv_led_off(heartbeat);
        }
    }
}

