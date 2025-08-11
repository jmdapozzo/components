#include <ledPanel.hpp>
#include <graphics.hpp>
#include <esp_log.h>
#include <logo.h>
#include <font.h>

static const char *TAG = "mbi5026";

static lv_anim_t _animationTemplate;
static lv_style_t _styleSmallPrimary;
static lv_style_t _styleMediumPrimary;
static lv_style_t _styleMediumAnimation;

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "LED Panel Test Program");

    macdap::Graphics &graphics = macdap::Graphics::getInstance();

    macdap::LedPanel &ledPanel = macdap::LedPanel::getInstance();
    lv_display_t *display = ledPanel.getLvDisplay();
    graphics.init(display);

    lv_anim_init(&_animationTemplate);
    lv_anim_set_delay(&_animationTemplate, 1000);
    lv_anim_set_repeat_delay(&_animationTemplate, 3000);
    lv_anim_set_repeat_count(&_animationTemplate, LV_ANIM_REPEAT_INFINITE);

    lv_style_init(&_styleSmallPrimary);
    lv_style_set_text_font(&_styleSmallPrimary, &nothingFont5x7);
    lv_style_set_text_color(&_styleSmallPrimary, lv_color_white());

    lv_style_init(&_styleMediumPrimary);
    lv_style_set_text_font(&_styleMediumPrimary, &lv_font_unscii_8);
    lv_style_set_text_color(&_styleMediumPrimary, lv_color_white());

    lv_style_init(&_styleMediumAnimation);
    lv_style_set_text_font(&_styleMediumAnimation, &lv_font_unscii_8);
    lv_style_set_text_color(&_styleMediumAnimation, lv_color_white());
    lv_style_set_anim(&_styleMediumAnimation, &_animationTemplate);

    graphics.background(display, lv_color_black());

    float brightness = 1.0;
    ledPanel.setBrightness(brightness);

    graphics.logo(display, &colorLogoNoText16x16);
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(display);

    graphics.greeting(display, &_styleSmallPrimary, "mbi5026", "0.0.0");
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(display);

    graphics.message(display, &_styleMediumPrimary, "MacDap");
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(display);

    graphics.scrollingMessageCenter(display, &_styleMediumAnimation, "Test program x64y32, Version 0.0.0 from MacDap Inc.");

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
