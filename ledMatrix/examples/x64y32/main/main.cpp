#include <ledMatrix.hpp>
#include <string.h>
// #include <esp_log.h>
#include <logo.h>
#include <heapHelper.h>

static const char *TAG = "x64y32";
static lv_obj_t *_scr;
static lv_coord_t _width;
static lv_coord_t _height;

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


void clear()
{
    if (lvgl_port_lock(0))
    {
        lv_obj_clean(_scr);
        lvgl_port_unlock();
    }
}

void background(lv_palette_t color)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_set_style_bg_color(_scr,lv_palette_main(color),LV_PART_MAIN);
        lvgl_port_unlock();
    }
}

void logo(void)
{
    if (lvgl_port_lock(0)) 
    {
        lv_obj_t *logo = lv_img_create(_scr);
        lv_img_set_src(logo, &colorLogoNoText64x64);
        lv_obj_align(logo, LV_ALIGN_LEFT_MID, 0, 0);

        lvgl_port_unlock();
    }
}

void greeting(const char *projectName, const char *version)
{
    if (lvgl_port_lock(0)) 
    {
        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_text_font(&style, &lv_font_montserrat_14);
        lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_AMBER));
        lv_style_set_align(&style, LV_ALIGN_CENTER);

        lv_obj_t *labelProjectName = lv_label_create(_scr);
        lv_obj_add_style(labelProjectName, &style, LV_STATE_DEFAULT);
        lv_label_set_text(labelProjectName, projectName);
        lv_obj_align(labelProjectName, LV_ALIGN_TOP_LEFT, 1, 0);

        lv_obj_t *labelVersion = lv_label_create(_scr);
        lv_obj_add_style(labelVersion, &style, LV_STATE_DEFAULT);
        lv_label_set_text(labelVersion, version);
        lv_obj_align(labelVersion, LV_ALIGN_BOTTOM_LEFT, -1, 0);

        lvgl_port_unlock();
    }
}

void message(const char *message)
{
    if (lvgl_port_lock(0))
    {
        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_text_font(&style, &lv_font_montserrat_20);
        lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_GREEN));
        lv_style_set_align(&style, LV_ALIGN_CENTER);

        lv_obj_t *label = lv_label_create(_scr);
        lv_obj_add_style(label, &style, LV_STATE_DEFAULT);

        lv_label_set_text(label, message);
        lvgl_port_unlock();
    }
}

void scrollingMessage1(const char *message)
{
    if (lvgl_port_lock(0))
    {
        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_text_font(&style, &lv_font_montserrat_18);
        lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_GREEN));
        lv_style_set_align(&style, LV_ALIGN_BOTTOM_MID);

        lv_obj_t *label = lv_label_create(_scr);
        lv_obj_add_style(label, &style, LV_STATE_DEFAULT);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(label, _width);
        lv_label_set_text(label, message);

        lvgl_port_unlock();
    }
}

void scrollingMessage2(const char *message)
{
    static lv_obj_t *label = nullptr;

    if (lvgl_port_lock(0))
    {
        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_text_font(&style, &lv_font_montserrat_14);
        // lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_NONE));
        lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_RED));
        lv_style_set_align(&style, LV_ALIGN_TOP_RIGHT);

        label = lv_label_create(_scr);
        lv_obj_add_style(label, &style, LV_STATE_DEFAULT);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(label, _width);
        lv_label_set_text(label, message);

        lvgl_port_unlock();
    }
}

void scrollingMessage3(const char *message)
{
    static lv_anim_t animationTemplate;
    static lv_style_t style;

    lv_anim_init(&animationTemplate);
    lv_anim_set_delay(&animationTemplate, 1000);
    lv_anim_set_repeat_delay(&animationTemplate, 3000);

    lv_style_init(&style);
    lv_style_set_text_font(&style, &lv_font_montserrat_18);
    lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_align(&style, LV_ALIGN_CENTER);
    lv_style_set_anim(&style, &animationTemplate);

    lv_obj_t * label1 = lv_label_create(_scr);
    lv_obj_add_style(label1, &style, LV_STATE_DEFAULT);
    lv_label_set_long_mode(label1, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(label1, _width);
    lv_label_set_text(label1, message);
}

void qrcode(void)
{
    if (lvgl_port_lock(0))
    {
        lv_color_t bgColor = lv_palette_lighten(LV_PALETTE_NONE, 5);
        lv_color_t fgColor = lv_palette_darken(LV_PALETTE_AMBER, 4);

        lv_obj_t *qr = lv_qrcode_create(_scr);
        lv_qrcode_set_size(qr, _height);
        lv_qrcode_set_dark_color(qr, fgColor);
        lv_qrcode_set_light_color(qr, bgColor);

        const char * data = "https://macdap.net";
        lv_qrcode_update(qr, data, strlen(data));
        lv_obj_center(qr);
        lv_obj_align(qr, LV_ALIGN_LEFT_MID, 0, 0);

        lvgl_port_unlock();
    }
}

void cross(void)
{
    if (lvgl_port_lock(0))
    {
        static lv_point_precise_t line1Points[] = { {0, 0}, {CONFIG_LED_MATRIX_MODULE_WIDTH * CONFIG_LED_MATRIX_PIXEL_WIDTH, CONFIG_LED_MATRIX_MODULE_HEIGHT * CONFIG_LED_MATRIX_PIXEL_HEIGHT} };
        static lv_point_precise_t line2Points[] = { {0, CONFIG_LED_MATRIX_MODULE_HEIGHT * CONFIG_LED_MATRIX_PIXEL_HEIGHT}, {CONFIG_LED_MATRIX_MODULE_WIDTH * CONFIG_LED_MATRIX_PIXEL_WIDTH, 0} };

        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_line_width(&style, 3);
        lv_style_set_line_color(&style, lv_palette_main(LV_PALETTE_BLUE));
        lv_style_set_line_rounded(&style, true);

        lv_obj_t * line1;
        line1 = lv_line_create(_scr);
        lv_line_set_points(line1, line1Points, 2);
        lv_obj_add_style(line1, &style, LV_STATE_DEFAULT);

        lv_obj_t * line2;
        line2 = lv_line_create(_scr);
        lv_line_set_points(line2, line2Points, 2);
        lv_obj_add_style(line2, &style, LV_STATE_DEFAULT);

        lvgl_port_unlock();
    }
}

void spinner(void)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_arc_color(&style, lv_palette_main(LV_PALETTE_RED));

    lv_obj_t *spinner = lv_spinner_create(_scr);
    lv_obj_add_style(spinner, &style, LV_STATE_DEFAULT);
    // lv_spinner_set_anim_params(spinner, 1000, 60);
    lv_obj_set_size(spinner, _height, _height);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 0);
}

lv_obj_t *led(void)
{
    lv_obj_t *led = nullptr;

    if (lvgl_port_lock(0))
    {
        led = lv_led_create(_scr);
        lv_obj_set_size(led, 16, 16);
        // lv_obj_set_size(led, 8, 8);
        lv_obj_align(led, LV_ALIGN_CENTER, 0, 0);
        //lv_obj_align(led, LV_ALIGN_TOP_RIGHT, -8, 4);
        lv_led_set_color(led, lv_palette_main(LV_PALETTE_RED));
        lv_led_off(led);

        lvgl_port_unlock();
    }
    return led;
}

extern "C" void app_main(void)
{
    // ESP_LOGI(TAG, "LED Matrix Test Program");

    const lvgl_port_cfg_t lvglConfig = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvglConfig);

    macdap::LedMatrix &ledMatrix = macdap::LedMatrix::getInstance();
    lv_disp_t *display = ledMatrix.getLvDisp();
    _scr = lv_disp_get_scr_act(display);
    _width = lv_disp_get_hor_res(display);
    _height = lv_disp_get_ver_res(display);
    background(LV_PALETTE_NONE);
    // ESP_LOGI(TAG, "Display resolution: %d x %d", _width, _height);

    float brightness = 100.0;
    ledMatrix.setBrightness(brightness);

    logo();
    vTaskDelay(pdMS_TO_TICKS(3000));
    clear();

    greeting("Test", "Version 0.0.0");
    vTaskDelay(pdMS_TO_TICKS(3000));
    clear();

    message("MacDap Inc.");
    vTaskDelay(pdMS_TO_TICKS(3000));
    clear();

    //brightness = 10.0;
    //ledMatrix.setBrightness(brightness);

    qrcode();
    vTaskDelay(pdMS_TO_TICKS(3000));
    clear();

    cross();
    vTaskDelay(pdMS_TO_TICKS(3000));
    clear();

    scrollingMessage3("This is a MacDap application!");
    vTaskDelay(pdMS_TO_TICKS(10000));
    clear();

    spinner();
    vTaskDelay(pdMS_TO_TICKS(5000));
    clear();

    //brightness = 1.0;
    //ledMatrix.setBrightness(brightness);
    lv_obj_t *heartbeat = led();
    scrollingMessage1("Test program, Version 0.0.0 from MacDap Inc.");
    scrollingMessage2("MacDap Inc. the best");

#ifdef CONFIG_HEAP_TASK_TRACKING
    esp_dump_per_task_heap_info();
#endif

    int palette_idx = 0;
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

