#include <graphics.hpp>
#include <string.h>
#include <esp_log.h>

using namespace macdap;

static const char *TAG = "graphics";

Graphics::Graphics()
{
    ESP_LOGI(TAG, "Initializing...");

    const lvgl_port_cfg_t lvglConfig = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvglConfig);
}

Graphics::~Graphics()
{
}

esp_err_t Graphics::init(lv_display_t *display)
{
    ESP_LOGI(TAG, "Init display");

    if (display == nullptr)
    {
        ESP_LOGE(TAG, "Display is null");
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

void Graphics::clear(lv_display_t *display)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);

        lv_obj_clean(scr);
        lvgl_port_unlock();
    }
}

void Graphics::background(lv_display_t *display, lv_color_t color)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);

        lv_obj_set_style_bg_color(scr, color, LV_PART_MAIN);
        lvgl_port_unlock();
    }
}

void Graphics::logo(lv_display_t *display, const void *src)
{
    if (lvgl_port_lock(0)) 
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);

        lv_obj_t *logo = lv_image_create(scr);
        lv_image_set_src(logo, src);
        lv_obj_align(logo, LV_ALIGN_LEFT_MID, 0, 0);

        lvgl_port_unlock();
    }
}

void Graphics::greeting(lv_display_t *display, lv_style_t *style, const char *projectName, const char *version)
{
    if (lvgl_port_lock(0)) 
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);

        lv_obj_t *labelProjectName = lv_label_create(scr);
        lv_obj_add_style(labelProjectName, style, LV_STATE_DEFAULT);
        lv_label_set_text(labelProjectName, projectName);
        lv_obj_align(labelProjectName, LV_ALIGN_TOP_LEFT, 0, 0);

        lv_obj_t *labelVersion = lv_label_create(scr);
        lv_obj_add_style(labelVersion, style, LV_STATE_DEFAULT);
        lv_label_set_text(labelVersion, version);
        lv_obj_align(labelVersion, LV_ALIGN_BOTTOM_LEFT, 0, 0);

        lvgl_port_unlock();
    }
}

lv_obj_t *Graphics::message(lv_display_t *display, lv_style_t *style, const char *message)
{
    lv_obj_t *label = nullptr;

    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);

        label = lv_label_create(scr);
        lv_obj_add_style(label, style, LV_STATE_DEFAULT);
        lv_label_set_text(label, message);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

        lvgl_port_unlock();
    }
    return label;
}

lv_obj_t *Graphics::scrollingMessageTop(lv_display_t *display, lv_style_t *style, const char *message)
{
    lv_obj_t *label = nullptr;

    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);
        int32_t width = lv_display_get_horizontal_resolution(display);

        label = lv_label_create(scr);
        lv_obj_add_style(label, style, LV_STATE_DEFAULT);
        lv_obj_align(label, LV_ALIGN_TOP_RIGHT, 0, 0);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(label, width);
        lv_label_set_text(label, message);

        lvgl_port_unlock();
    }
    return label;
}

lv_obj_t *Graphics::scrollingMessageCenter(lv_display_t *display, lv_style_t *style, const char *message)
{
    lv_obj_t *label = nullptr;

    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);
        int32_t width = lv_display_get_horizontal_resolution(display);

        label = lv_label_create(scr);
        lv_obj_add_style(label, style, LV_STATE_DEFAULT);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(label, width);
        lv_label_set_text(label, message);

        lvgl_port_unlock();
    }
    return label;
}

lv_obj_t *Graphics::scrollingMessageBottom(lv_display_t *display, lv_style_t *style, const char *message)
{
    lv_obj_t *label = nullptr;

    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);
        int32_t width = lv_display_get_horizontal_resolution(display);

        label = lv_label_create(scr);
        lv_obj_add_style(label, style, LV_STATE_DEFAULT);
        lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(label, width);
        lv_label_set_text(label, message);

        lvgl_port_unlock();
    }
    return label;
}

void Graphics::qrcode(lv_display_t *display)
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

void Graphics::cross(lv_display_t *display)
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

void Graphics::spinner(lv_display_t *display)
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

lv_obj_t *Graphics::led(lv_display_t *display)
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
