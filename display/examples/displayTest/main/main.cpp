#include <display.hpp>
#include <graphics.hpp>
#include <driver/i2c_master.h>
#include <string.h>
#include <esp_log.h>
#include <logo.h>

static const char *TAG = "displayTest";

static lv_anim_t _animationTemplate;
static lv_style_t _styleSmallPrimary;
static lv_style_t _styleSmallAnimation;

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

    lv_anim_init(&_animationTemplate);
    lv_anim_set_delay(&_animationTemplate, 1000);
    lv_anim_set_repeat_delay(&_animationTemplate, 3000);
    lv_anim_set_repeat_count(&_animationTemplate, LV_ANIM_REPEAT_INFINITE);

    lv_style_init(&_styleSmallPrimary);
    lv_style_set_text_font(&_styleSmallPrimary, &lv_font_montserrat_14);
    lv_style_set_text_color(&_styleSmallPrimary, lv_color_black());

    lv_style_init(&_styleSmallAnimation);
    lv_style_set_text_font(&_styleSmallAnimation, &lv_font_montserrat_14);
    lv_style_set_text_color(&_styleSmallAnimation, lv_color_black());

    graphics.background(lvDisplay, lv_color_white());

    graphics.logo(lvDisplay, &colorLogoNoText64x64);
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.greeting(lvDisplay, &_styleSmallPrimary, "Test", "Version 0.0.0");
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.message(lvDisplay, &_styleSmallPrimary, "MacDap Inc.");
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.qrcode(lvDisplay);
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.cross(lvDisplay, lv_color_black());
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.scrollingMessageCenter(lvDisplay, &_styleSmallAnimation, "This is a MacDap application!");
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.spinner(lvDisplay);
    vTaskDelay(pdMS_TO_TICKS(5000));
    graphics.clear(lvDisplay);

    // TODO Toggling does not work
    lv_obj_t *heartbeat = graphics.led(lvDisplay, lv_color_black());

    graphics.scrollingMessageBottom(lvDisplay, &_styleSmallAnimation, "Test program Version 0.0.0 from MacDap Inc.");
    graphics.scrollingMessageTop(lvDisplay, &_styleSmallAnimation, "MacDap Inc. the best");

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

