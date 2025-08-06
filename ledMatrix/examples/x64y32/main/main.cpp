#include <ledMatrix.hpp>
#include <string.h>
#include <esp_log.h>
#include <logo.h>
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

void clear(lv_display_t *display)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);

        lv_obj_clean(scr);
        lvgl_port_unlock();
    }
}

void background(lv_display_t *display,lv_palette_t color)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);

        lv_obj_set_style_bg_color(scr,lv_palette_main(color),LV_PART_MAIN);
        lvgl_port_unlock();
    }
}

void logo(lv_display_t *display)
{
    if (lvgl_port_lock(0)) 
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);

        lv_obj_t *logo = lv_image_create(scr);
        lv_image_set_src(logo, &colorLogoNoText64x64);
        lv_obj_align(logo, LV_ALIGN_LEFT_MID, 0, 0);

        lvgl_port_unlock();
    }
}

void greeting(lv_display_t *display, const char *projectName, const char *version)
{
    if (lvgl_port_lock(0)) 
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);

        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_text_font(&style, &lv_font_montserrat_14);
        lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_AMBER));
        lv_style_set_align(&style, LV_ALIGN_CENTER);

        lv_obj_t *labelProjectName = lv_label_create(scr);
        lv_obj_add_style(labelProjectName, &style, LV_STATE_DEFAULT);
        lv_label_set_text(labelProjectName, projectName);
        lv_obj_align(labelProjectName, LV_ALIGN_TOP_LEFT, 1, 0);

        lv_obj_t *labelVersion = lv_label_create(scr);
        lv_obj_add_style(labelVersion, &style, LV_STATE_DEFAULT);
        lv_label_set_text(labelVersion, version);
        lv_obj_align(labelVersion, LV_ALIGN_BOTTOM_LEFT, -1, 0);

        lvgl_port_unlock();
    }
}

void message(lv_display_t *display, const char *message)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);

        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_text_font(&style, &lv_font_montserrat_28);
        lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_GREEN));

        lv_obj_t *label = lv_label_create(scr);
        lv_obj_add_style(label, &style, LV_STATE_DEFAULT);
        lv_label_set_text(label, message);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

        lvgl_port_unlock();
    }
}

void scrollingMessageTop(lv_display_t *display, const char *message)
{
    static lv_obj_t *label = nullptr;

    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);
        int32_t width = lv_display_get_horizontal_resolution(display);

        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_text_font(&style, &lv_font_montserrat_20);
        // lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_NONE));
        lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_RED));
        lv_style_set_align(&style, LV_ALIGN_TOP_RIGHT);

        label = lv_label_create(scr);
        lv_obj_add_style(label, &style, LV_STATE_DEFAULT);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(label, width);
        lv_label_set_text(label, message);

        lvgl_port_unlock();
    }
}

void scrollingMessageCenter(lv_display_t *display, const char *message)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);
        int32_t width = lv_display_get_horizontal_resolution(display);

        static lv_anim_t animationTemplate;
        lv_anim_init(&animationTemplate);
        lv_anim_set_delay(&animationTemplate, 1000);
        lv_anim_set_repeat_delay(&animationTemplate, 3000);

        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_text_font(&style, &lv_font_montserrat_24);
        lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_GREEN));
        lv_style_set_align(&style, LV_ALIGN_CENTER);
        lv_style_set_anim(&style, &animationTemplate);

        lv_obj_t * label1 = lv_label_create(scr);
        lv_obj_add_style(label1, &style, LV_STATE_DEFAULT);
        lv_label_set_long_mode(label1, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(label1, width);
        lv_label_set_text(label1, message);

        lvgl_port_unlock();
    }
}

void scrollingMessageBottom(lv_display_t *display, const char *message)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);
        int32_t width = lv_display_get_horizontal_resolution(display);

        static lv_anim_t animationTemplate;
        lv_anim_init(&animationTemplate);
        lv_anim_set_delay(&animationTemplate, 1000);
        lv_anim_set_repeat_delay(&animationTemplate, 3000);
        lv_anim_set_repeat_count(&animationTemplate, LV_ANIM_REPEAT_INFINITE);

        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_text_font(&style, &lv_font_montserrat_24);
        lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_GREEN));
        lv_style_set_align(&style, LV_ALIGN_BOTTOM_MID);
        lv_style_set_anim(&style, &animationTemplate);

        lv_obj_t *label = lv_label_create(scr);
        lv_obj_add_style(label, &style, LV_STATE_DEFAULT);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(label, width);
        lv_label_set_text(label, message);

        lvgl_port_unlock();
    }
}

void qrcode(lv_display_t *display)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);
        int32_t height = lv_display_get_vertical_resolution(display);

        lv_color_t bgColor = lv_palette_lighten(LV_PALETTE_NONE, 5);
        lv_color_t fgColor = lv_palette_darken(LV_PALETTE_AMBER, 4);

        lv_obj_t *qr = lv_qrcode_create(scr);
        lv_qrcode_set_size(qr, height);
        lv_qrcode_set_dark_color(qr, fgColor);
        lv_qrcode_set_light_color(qr, bgColor);

        const char *data = "https://macdap.net";
        lv_qrcode_update(qr, data, strlen(data));
        lv_obj_center(qr);
        lv_obj_align(qr, LV_ALIGN_LEFT_MID, 0, 0);

        lvgl_port_unlock();
    }
}

void cross(lv_display_t *display)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);

        int32_t width = lv_display_get_horizontal_resolution(display);
        int32_t height = lv_display_get_vertical_resolution(display);

        static lv_point_precise_t line1Points[] = { {0, 0}, {width, height} };
        static lv_point_precise_t line2Points[] = { {0, height}, {width, 0} };

        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_line_width(&style, 3);
        lv_style_set_line_color(&style, lv_palette_main(LV_PALETTE_BLUE));
        lv_style_set_line_rounded(&style, true);

        lv_obj_t *line1;
        line1 = lv_line_create(scr);
        lv_line_set_points(line1, line1Points, 2);
        lv_obj_add_style(line1, &style, LV_STATE_DEFAULT);

        lv_obj_t *line2;
        line2 = lv_line_create(scr);
        lv_line_set_points(line2, line2Points, 2);
        lv_obj_add_style(line2, &style, LV_STATE_DEFAULT);

        lvgl_port_unlock();
    }
}

void spinner(lv_display_t *display)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);
        int32_t height = lv_display_get_vertical_resolution(display);

        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_BLUE));
        lv_style_set_arc_color(&style, lv_palette_main(LV_PALETTE_RED));

        lv_obj_t *spinner = lv_spinner_create(scr);
        lv_obj_add_style(spinner, &style, LV_STATE_DEFAULT);
        // lv_spinner_set_anim_params(spinner, 1000, 60);
        lv_obj_set_size(spinner, height, height);
        lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 0);

        lvgl_port_unlock();
    }
}

lv_obj_t *led(lv_display_t *display)
{
    lv_obj_t *led = nullptr;

    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);

        led = lv_led_create(scr);
        lv_obj_set_size(led, 32, 32);
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
    ESP_LOGI(TAG, "LED Matrix Test Program");

    const lvgl_port_cfg_t lvglConfig = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvglConfig);

    macdap::LedMatrix &ledMatrix = macdap::LedMatrix::getInstance();
    lv_display_t *display = ledMatrix.getLvDisplay();

    background(display, LV_PALETTE_NONE);

    float brightness = 100.0;
    ledMatrix.setBrightness(brightness);

    logo(display);
    vTaskDelay(pdMS_TO_TICKS(3000));
    clear(display);

    greeting(display, "Test", "Version 0.0.0");
    vTaskDelay(pdMS_TO_TICKS(3000));
    clear(display);

    message(display, "MacDap Inc.");
    vTaskDelay(pdMS_TO_TICKS(3000));
    clear(display);

    //brightness = 10.0;
    //ledMatrix.setBrightness(brightness);

    qrcode(display);
    vTaskDelay(pdMS_TO_TICKS(3000));
    clear(display);

    cross(display);
    vTaskDelay(pdMS_TO_TICKS(3000));
    clear(display);

    scrollingMessageCenter(display, "This is a MacDap application!");
    vTaskDelay(pdMS_TO_TICKS(10000));
    clear(display);

    spinner(display);
    vTaskDelay(pdMS_TO_TICKS(5000));
    clear(display);

    //brightness = 1.0;
    //ledMatrix.setBrightness(brightness);
    lv_obj_t *heartbeat = led(display);
    scrollingMessageBottom(display, "Test program Version 0.0.0 from MacDap Inc.");
    scrollingMessageTop(display, "MacDap Inc. the best");

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

