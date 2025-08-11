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

static lv_color_t _primaryColor = lv_palette_main(LV_PALETTE_GREEN);
static lv_color_t _secondaryColor = lv_palette_main(LV_PALETTE_RED);
static lv_color_t _discreteColor = lv_palette_main(LV_PALETTE_AMBER);

static lv_anim_t _animationTemplate;
static lv_style_t _styleSmallPrimary;
static lv_style_t _styleMediumPrimary;
static lv_style_t _styleLargePrimary;
static lv_style_t _styleLargeAnimation;
static lv_style_t _styleExtraLargePrimary;

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "LED Matrix Test Program");

    macdap::Graphics &graphics = macdap::Graphics::getInstance();

    macdap::LedMatrix &ledMatrix = macdap::LedMatrix::getInstance();
    lv_display_t *lvDisplay = ledMatrix.getLvDisplay();
    graphics.init(lvDisplay);

    lv_anim_init(&_animationTemplate);
    lv_anim_set_delay(&_animationTemplate, 1000);
    lv_anim_set_repeat_delay(&_animationTemplate, 3000);
    lv_anim_set_repeat_count(&_animationTemplate, LV_ANIM_REPEAT_INFINITE);

    lv_style_init(&_styleSmallPrimary);
    lv_style_set_text_font(&_styleSmallPrimary, &lv_font_montserrat_14);
    lv_style_set_text_color(&_styleSmallPrimary, _discreteColor);

    lv_style_init(&_styleMediumPrimary);
    lv_style_set_text_font(&_styleMediumPrimary, &lv_font_montserrat_20);
    lv_style_set_text_color(&_styleMediumPrimary, _secondaryColor);

    lv_style_init(&_styleLargePrimary);
    lv_style_set_text_font(&_styleLargePrimary, &lv_font_montserrat_24);
    lv_style_set_text_color(&_styleLargePrimary, _primaryColor);

    lv_style_init(&_styleLargeAnimation);
    lv_style_set_text_font(&_styleLargeAnimation, &lv_font_montserrat_24);
    lv_style_set_text_color(&_styleLargeAnimation, _primaryColor);
    lv_style_set_anim(&_styleLargeAnimation, &_animationTemplate);

    lv_style_init(&_styleExtraLargePrimary);
    lv_style_set_text_font(&_styleExtraLargePrimary, &lv_font_montserrat_28);
    lv_style_set_text_color(&_styleExtraLargePrimary, _primaryColor);
   
    graphics.background(lvDisplay, lv_color_black());

    float brightness = 100.0;
    ledMatrix.setBrightness(brightness);

    graphics.logo(lvDisplay, &colorLogoNoText64x64);
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.greeting(lvDisplay, &_styleSmallPrimary, "Test", "Version 0.0.0");
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.message(lvDisplay, &_styleExtraLargePrimary, "MacDap Inc.");
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.qrcode(lvDisplay);
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.cross(lvDisplay, lv_palette_main(LV_PALETTE_BLUE));
    vTaskDelay(pdMS_TO_TICKS(3000));
    graphics.clear(lvDisplay);

    graphics.scrollingMessageCenter(lvDisplay, &_styleLargeAnimation, "This is a MacDap application!");
    vTaskDelay(pdMS_TO_TICKS(10000));
    graphics.clear(lvDisplay);

    graphics.spinner(lvDisplay);
    vTaskDelay(pdMS_TO_TICKS(5000));
    graphics.clear(lvDisplay);

    int palette_idx = 0;
    lv_obj_t *heartbeat = graphics.led(lvDisplay, lv_palette_main(palettes[palette_idx]));
    graphics.scrollingMessageBottom(lvDisplay, &_styleLargeAnimation, "Test program Version 0.0.0 from MacDap Inc.");
    graphics.scrollingMessageTop(lvDisplay, &_styleMediumPrimary, "MacDap Inc. the best");

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
            lv_led_on(heartbeat);
        }
        else
        {
            phase = true;
            lv_led_off(heartbeat);
        }

        tick++;
        if (tick >= 4)
        {
            tick = 0;
            palette_idx = (palette_idx + 1) % (sizeof(palettes)/sizeof(palettes[0]));
            lv_led_set_color(heartbeat, lv_palette_main(palettes[palette_idx]));
        }
    }
}

