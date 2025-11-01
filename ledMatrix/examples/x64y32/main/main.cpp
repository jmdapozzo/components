#include <ledMatrix.hpp>
#include <graphics.hpp>
#include <string.h>
#include <esp_log.h>
#include <heapHelper.h>

static const char *TAG = "x64y32";

static lv_palette_t palettes[] = {
    LV_PALETTE_RED,
    LV_PALETTE_PINK,
    LV_PALETTE_PURPLE,
    LV_PALETTE_DEEP_PURPLE,
    LV_PALETTE_INDIGO,
    LV_PALETTE_BLUE,
    LV_PALETTE_LIGHT_BLUE,
    LV_PALETTE_CYAN,
    LV_PALETTE_TEAL,
    LV_PALETTE_GREEN,
    LV_PALETTE_LIGHT_GREEN,
    LV_PALETTE_LIME,
    LV_PALETTE_YELLOW,
    LV_PALETTE_AMBER,
    LV_PALETTE_ORANGE,
    LV_PALETTE_DEEP_ORANGE,
    LV_PALETTE_BROWN,
    LV_PALETTE_BLUE_GREY,
    LV_PALETTE_GREY,
};

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

    macdap::Graphics &graphics = macdap::Graphics::get_instance();

    macdap::LedMatrix &ledMatrix = macdap::LedMatrix::get_instance();
    lv_display_t *lv_display = ledMatrix.get_lv_display();
    graphics.init(lv_display);

    ESP_LOGI(TAG, "Free heap: %d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    int32_t width = lv_display_get_horizontal_resolution(lv_display);
    int32_t height = lv_display_get_vertical_resolution(lv_display);

    lv_anim_init(&_animation);
    lv_anim_set_delay(&_animation, 1000);
    lv_anim_set_repeat_delay(&_animation, 3000);
    lv_anim_set_repeat_count(&_animation, LV_ANIM_REPEAT_INFINITE);

    lv_style_init(&_style_base);
    lv_style_set_bg_color(&_style_base, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_arc_color(&_style_base, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_text_align(&_style_base, LV_TEXT_ALIGN_CENTER);
    lv_style_set_align(&_style_base, LV_ALIGN_CENTER);
    lv_style_set_text_color(&_style_base, lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_width(&_style_base, width);
    lv_style_set_line_width(&_style_base, 3);
    lv_style_set_line_color(&_style_base, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_line_rounded(&_style_base, true);
    lv_style_set_anim(&_style_base, &_animation);

    lv_style_copy(&_style_greeting_top, &_style_base);
    lv_style_set_text_font(&_style_greeting_top, &lv_font_montserrat_14);
    lv_style_set_text_color(&_style_greeting_top, lv_palette_main(LV_PALETTE_AMBER));
    lv_style_set_align(&_style_greeting_top, LV_ALIGN_TOP_LEFT);

    lv_style_copy(&_style_greeting_bottom, &_style_base);
    lv_style_set_text_font(&_style_greeting_bottom, &lv_font_montserrat_14);
    lv_style_set_text_color(&_style_greeting_bottom, lv_palette_main(LV_PALETTE_AMBER));
    lv_style_set_align(&_style_greeting_bottom, LV_ALIGN_BOTTOM_LEFT);

    lv_style_copy(&_style_main_message, &_style_base);
    lv_style_set_text_font(&_style_main_message, &lv_font_montserrat_24);

    lv_style_copy(&_style_main_message_top, &_style_main_message);
    lv_style_set_align(&_style_main_message_top, LV_ALIGN_TOP_LEFT);
    lv_style_set_text_color(&_style_main_message_top, lv_palette_main(LV_PALETTE_GREEN));

    lv_style_copy(&_style_main_message_bottom, &_style_main_message);
    lv_style_set_align(&_style_main_message_bottom, LV_ALIGN_BOTTOM_LEFT);
    lv_style_set_text_color(&_style_main_message_bottom, lv_palette_main(LV_PALETTE_RED));

    graphics.background(lv_display, lv_color_black());

    float brightness = 1.0;
    ledMatrix.set_brightness(brightness);

    graphics.logo(lv_display, &colorLogoNoText64x64);
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lv_display);

    graphics.message(lv_display, &_style_greeting_top, "Test");
    graphics.message(lv_display, &_style_greeting_bottom, "Version 0.0.0");
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lv_display);

    graphics.message(lv_display, &_style_main_message, "MacDap Inc.");
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lv_display);

    graphics.qrcode(lv_display, "https://macdap.com");
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lv_display);

    graphics.cross(lv_display, &_style_base);
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lv_display);

    graphics.message(lv_display, &_style_main_message, "This is a MacDap application!");
    vTaskDelay(pdMS_TO_TICKS(5000));
    graphics.clear(lv_display);

    graphics.spinner(lv_display, height/2, &_style_base);
    vTaskDelay(pdMS_TO_TICKS(5000));
    graphics.clear(lv_display);

    int palette_idx = 0;
    lv_obj_t *heartbeat = graphics.led(lv_display, 32, lv_palette_main(palettes[palette_idx]));

    graphics.message(lv_display, &_style_main_message_top, "Test program Version 0.0.0 from MacDap Inc.");
    graphics.message(lv_display, &_style_main_message_bottom, "MacDap Inc. the best");

#ifdef CONFIG_HEAP_TASK_TRACKING
    esp_dump_per_task_heap_info();
#endif

    int tick = 0;
    bool phase = false;
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        if (phase)
        {
            phase = false;
            if (graphics.seize_lvgl())
            {
                lv_led_on(heartbeat);
                graphics.release_lvgl();
            }
        }
        else
        {
            phase = true;
            if (graphics.seize_lvgl())
            {
                lv_led_off(heartbeat);
                graphics.release_lvgl();
            }
        }

        tick++;
        if (tick >= 4)
        {
            tick = 0;
            palette_idx = (palette_idx + 1) % (sizeof(palettes)/sizeof(palettes[0]));
            if (lvgl_port_lock(0))
            {
                lv_led_set_color(heartbeat, lv_palette_main(palettes[palette_idx]));
                lvgl_port_unlock();
            }
        }
    }
}

