#include <display.hpp>
#include <graphics.hpp>
#include <driver/i2c_master.h>
#include <string.h>
#include <esp_log.h>
#include <logo.h>

static const char *TAG = "displayTest";

static lv_anim_t _animation;
static lv_style_t _styleBase;
static lv_style_t _styleGreetingTop;
static lv_style_t _styleGreetingBottom;
static lv_style_t _styleMainMessage;
static lv_style_t _styleMainMessageTop;
static lv_style_t _styleMainMessageBottom;

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "LED Matrix Test Program");

    i2c_master_bus_config_t i2cMasterBusConfig = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = static_cast<gpio_num_t>(CONFIG_GPIO_I2C_SDA),
        .scl_io_num = static_cast<gpio_num_t>(CONFIG_GPIO_I2C_SCL),
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7
    };
    i2cMasterBusConfig.flags.enable_internal_pullup = true;

    i2c_master_bus_handle_t i2cMasterBusHandle = NULL;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2cMasterBusConfig, &i2cMasterBusHandle));

    ESP_LOGI(TAG, "Probing...");
    for (int i = 0; i < 0x80; i++)
    {
        if (i2c_master_probe(i2cMasterBusHandle, i, 100) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found device at 0x%02x", i);
        }
    }
    ESP_LOGI(TAG, "Done probing!");

    macdap::Graphics &graphics = macdap::Graphics::getInstance();

    macdap::Display &display = macdap::Display::getInstance();
    lv_disp_t *lvDisplay = display.getLvDisplay();
    graphics.init(lvDisplay);

    int32_t width = lv_display_get_horizontal_resolution(lvDisplay);
    int32_t height = lv_display_get_vertical_resolution(lvDisplay);

    lv_anim_init(&_animation);
    lv_anim_set_delay(&_animation, 1000);
    lv_anim_set_repeat_delay(&_animation, 3000);
    lv_anim_set_repeat_count(&_animation, LV_ANIM_REPEAT_INFINITE);

    lv_style_init(&_styleBase);
    lv_style_set_text_align(&_styleBase, LV_TEXT_ALIGN_CENTER);
    lv_style_set_align(&_styleBase, LV_ALIGN_CENTER);
    lv_style_set_text_color(&_styleBase, lv_color_black());
    lv_style_set_width(&_styleBase, width);
    lv_style_set_line_width(&_styleBase, 3);
    lv_style_set_line_color(&_styleBase, lv_color_black());
    lv_style_set_line_rounded(&_styleBase, true);
    lv_style_set_anim(&_styleBase, &_animation);

    lv_style_copy(&_styleGreetingTop, &_styleBase);
    lv_style_set_text_font(&_styleGreetingTop, &lv_font_montserrat_14);
    lv_style_set_align(&_styleGreetingTop, LV_ALIGN_TOP_LEFT);

    lv_style_copy(&_styleGreetingBottom, &_styleBase);
    lv_style_set_text_font(&_styleGreetingBottom, &lv_font_montserrat_14);
    lv_style_set_align(&_styleGreetingBottom, LV_ALIGN_BOTTOM_LEFT);

    lv_style_copy(&_styleMainMessage, &_styleBase);
    lv_style_set_text_font(&_styleMainMessage, &lv_font_montserrat_24);

    lv_style_copy(&_styleMainMessageTop, &_styleMainMessage);
    lv_style_set_align(&_styleMainMessageTop, LV_ALIGN_TOP_LEFT);

    lv_style_copy(&_styleMainMessageBottom, &_styleMainMessage);
    lv_style_set_align(&_styleMainMessageBottom, LV_ALIGN_BOTTOM_LEFT);

    graphics.background(lvDisplay, lv_color_white());

    graphics.logo(lvDisplay, &colorLogoNoText64x64);
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.message(lvDisplay, &_styleGreetingTop, "Test");
    graphics.message(lvDisplay, &_styleGreetingBottom, "Version 0.0.0");
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.message(lvDisplay, &_styleMainMessage, "MacDap Inc.");
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.qrcode(lvDisplay, "https://macdap.com");
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.cross(lvDisplay, &_styleBase);
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.message(lvDisplay, &_styleMainMessage, "This is a MacDap application!");
    vTaskDelay(pdMS_TO_TICKS(5000));
    graphics.clear(lvDisplay);

    graphics.spinner(lvDisplay, height/2, &_styleBase);
    vTaskDelay(pdMS_TO_TICKS(5000));
    graphics.clear(lvDisplay);

    // TODO Toggling does not work
    // lv_obj_t *heartbeat = graphics.led(lvDisplay, lv_color_black());

    graphics.message(lvDisplay, &_styleMainMessageTop, "Test program Version 0.0.0 from MacDap Inc.");
    graphics.message(lvDisplay, &_styleMainMessageBottom, "MacDap Inc. the best");

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

