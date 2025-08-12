#include <ledPanel.hpp>
#include <graphics.hpp>
#include <esp_log.h>
#include <logo.h>
#include <font.h>

static const char *TAG = "mbi5026";

static lv_anim_t _animation;
static lv_style_t _styleBase;
static lv_style_t _styleGreetingTop;
static lv_style_t _styleGreetingBottom;
static lv_style_t _styleMainMessage;

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "LED Panel Test Program");

    macdap::Graphics &graphics = macdap::Graphics::getInstance();

    macdap::LedPanel &ledPanel = macdap::LedPanel::getInstance();
    lv_display_t *lvDisplay = ledPanel.getLvDisplay();
    graphics.init(lvDisplay);

    int32_t width = lv_display_get_horizontal_resolution(lvDisplay);
    int32_t height = lv_display_get_vertical_resolution(lvDisplay);

    lv_anim_init(&_animation);
    lv_anim_set_delay(&_animation, 5000);
    lv_anim_set_repeat_delay(&_animation, 3000);
    lv_anim_set_repeat_count(&_animation, LV_ANIM_REPEAT_INFINITE);

    lv_style_init(&_styleBase);
    lv_style_set_text_color(&_styleBase, lv_color_white());
    lv_style_set_text_align(&_styleBase, LV_TEXT_ALIGN_CENTER);
    lv_style_set_align(&_styleBase, LV_ALIGN_CENTER);
    lv_style_set_width(&_styleBase, width);
    lv_style_set_anim(&_styleBase, &_animation);

    lv_style_copy(&_styleGreetingTop, &_styleBase);
    lv_style_set_text_font(&_styleGreetingTop, &nothingFont5x7);
    lv_style_set_align(&_styleGreetingTop, LV_ALIGN_TOP_LEFT);

    lv_style_copy(&_styleGreetingBottom, &_styleBase);
    lv_style_set_text_font(&_styleGreetingBottom, &nothingFont5x7);
    lv_style_set_align(&_styleGreetingBottom, LV_ALIGN_BOTTOM_LEFT);

    lv_style_copy(&_styleMainMessage, &_styleBase);
    lv_style_set_text_font(&_styleMainMessage, &lv_font_unscii_8);

    graphics.background(lvDisplay, lv_color_black());

    float brightness = 1.0;
    ledPanel.setBrightness(brightness);

    graphics.logo(lvDisplay, &colorLogoNoText16x16);
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.message(lvDisplay, &_styleGreetingTop, "mbi5026");
    graphics.message(lvDisplay, &_styleGreetingBottom, "0.0.0");
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.message(lvDisplay, &_styleMainMessage, "MacDap");
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.message(lvDisplay, &_styleMainMessage, "Test program x64y32, Version 0.0.0 from MacDap Inc.");

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
