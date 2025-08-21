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
static lv_style_t _styleBase;
static lv_style_t _styleGreetingTop;
static lv_style_t _styleGreetingBottom;
static lv_style_t _styleMainMessage;
static lv_style_t _styleMainMessageTop;
static lv_style_t _styleMainMessageBottom;

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "LED Matrix Test Program");

    macdap::Graphics &graphics = macdap::Graphics::getInstance();

    macdap::LedMatrix &ledMatrix = macdap::LedMatrix::getInstance();
    lv_display_t *lvDisplay = ledMatrix.getLvDisplay();
    graphics.init(lvDisplay);

    int32_t width = lv_display_get_horizontal_resolution(lvDisplay);
    int32_t height = lv_display_get_vertical_resolution(lvDisplay);

    lv_anim_init(&_animation);
    lv_anim_set_delay(&_animation, 1000);
    lv_anim_set_repeat_delay(&_animation, 3000);
    lv_anim_set_repeat_count(&_animation, LV_ANIM_REPEAT_INFINITE);

    lv_style_init(&_styleBase);
    lv_style_set_bg_color(&_styleBase, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_arc_color(&_styleBase, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_text_align(&_styleBase, LV_TEXT_ALIGN_CENTER);
    lv_style_set_align(&_styleBase, LV_ALIGN_CENTER);
    lv_style_set_text_color(&_styleBase, lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_width(&_styleBase, width);
    lv_style_set_line_width(&_styleBase, 3);
    lv_style_set_line_color(&_styleBase, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_line_rounded(&_styleBase, true);
    lv_style_set_anim(&_styleBase, &_animation);

    lv_style_copy(&_styleGreetingTop, &_styleBase);
    lv_style_set_text_font(&_styleGreetingTop, &lv_font_montserrat_14);
    lv_style_set_text_color(&_styleGreetingTop, lv_palette_main(LV_PALETTE_AMBER));
    lv_style_set_align(&_styleGreetingTop, LV_ALIGN_TOP_LEFT);

    lv_style_copy(&_styleGreetingBottom, &_styleBase);
    lv_style_set_text_font(&_styleGreetingBottom, &lv_font_montserrat_14);
    lv_style_set_text_color(&_styleGreetingBottom, lv_palette_main(LV_PALETTE_AMBER));
    lv_style_set_align(&_styleGreetingBottom, LV_ALIGN_BOTTOM_LEFT);

    lv_style_copy(&_styleMainMessage, &_styleBase);
    lv_style_set_text_font(&_styleMainMessage, &lv_font_montserrat_24);

    lv_style_copy(&_styleMainMessageTop, &_styleMainMessage);
    lv_style_set_align(&_styleMainMessageTop, LV_ALIGN_TOP_LEFT);
    lv_style_set_text_color(&_styleMainMessageTop, lv_palette_main(LV_PALETTE_GREEN));

    lv_style_copy(&_styleMainMessageBottom, &_styleMainMessage);
    lv_style_set_align(&_styleMainMessageBottom, LV_ALIGN_BOTTOM_LEFT);
    lv_style_set_text_color(&_styleMainMessageBottom, lv_palette_main(LV_PALETTE_RED));

    graphics.background(lvDisplay, lv_color_black());

    float brightness = 1.0;
    ledMatrix.setBrightness(brightness);

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

    int palette_idx = 0;
    lv_obj_t *heartbeat = graphics.led(lvDisplay, 32, lv_palette_main(palettes[palette_idx]));

    graphics.message(lvDisplay, &_styleMainMessageTop, "Test program Version 0.0.0 from MacDap Inc.");
    graphics.message(lvDisplay, &_styleMainMessageBottom, "MacDap Inc. the best");

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
            if (graphics.seizeLvgl())
            {
                lv_led_on(heartbeat);
                graphics.releaseLvgl();
            }
        }
        else
        {
            phase = true;
            if (graphics.seizeLvgl())
            {
                lv_led_off(heartbeat);
                graphics.releaseLvgl();
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

