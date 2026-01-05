#include <ledPanel.hpp>
#include <graphics.hpp>
#include <esp_log.h>

static const char *TAG = "mbi5026";

static lv_anim_t _animation;
static lv_style_t _style_base;
static lv_style_t _style_greeting_top;
static lv_style_t _style_greeting_bottom;
static lv_style_t _style_main_message;

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "LED Panel Test Program");

    macdap::Graphics &graphics = macdap::Graphics::get_instance();

    macdap::LedPanel &ledPanel = macdap::LedPanel::get_instance();
    lv_display_t *lv_display = ledPanel.get_lv_display();
    graphics.init(lv_display);

    int32_t width = lv_display_get_horizontal_resolution(lv_display);

    lv_anim_init(&_animation);
    lv_anim_set_delay(&_animation, 5000);
    lv_anim_set_repeat_delay(&_animation, 3000);
    lv_anim_set_repeat_count(&_animation, LV_ANIM_REPEAT_INFINITE);

    lv_style_init(&_style_base);
    lv_style_set_text_color(&_style_base, lv_color_white());
    lv_style_set_text_align(&_style_base, LV_TEXT_ALIGN_CENTER);
    lv_style_set_align(&_style_base, LV_ALIGN_CENTER);
    lv_style_set_width(&_style_base, width);
    lv_style_set_anim(&_style_base, &_animation);

    lv_style_copy(&_style_greeting_top, &_style_base);
    lv_style_set_text_font(&_style_greeting_top, &nothingFont5x7);
    lv_style_set_align(&_style_greeting_top, LV_ALIGN_TOP_LEFT);

    lv_style_copy(&_style_greeting_bottom, &_style_base);
    lv_style_set_text_font(&_style_greeting_bottom, &nothingFont5x7);
    lv_style_set_align(&_style_greeting_bottom, LV_ALIGN_BOTTOM_LEFT);

    lv_style_copy(&_style_main_message, &_style_base);
    lv_style_set_text_font(&_style_main_message, &lv_font_unscii_8);

    graphics.background(lv_display, lv_color_black());

    float brightness = 1.0;
    ledPanel.set_brightness(brightness);

    graphics.logo(lv_display, &colorLogoNoText16x16);
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lv_display);

    lv_obj_t *message = graphics.create_message(lv_display, "mbi5026", &_style_greeting_top);
    graphics.update_message(message, "0.0.0");
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lv_display);

    graphics.create_message(lv_display, "MacDap", &_style_main_message);
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lv_display);

    graphics.cross(lv_display);
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lv_display);

    graphics.create_message(lv_display, "Test program, Version 0.0.0 from MacDap Inc.", &_style_main_message);
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
