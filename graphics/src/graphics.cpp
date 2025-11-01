#include <graphics.hpp>
#include <string.h>
#include <esp_log.h>

using namespace macdap;

static const char *TAG = "graphics";

Graphics::Graphics()
{
    ESP_LOGI(TAG, "Initializing...");

    const lvgl_port_cfg_t lvgl_config = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_config);
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

bool Graphics::seize_lvgl(uint32_t ms_timeout)
{
    return lvgl_port_lock(ms_timeout);
}

void Graphics::release_lvgl(void)
{
    lvgl_port_unlock();
}

void Graphics::clear(lv_display_t *display)
{
    if (seize_lvgl())
    {
        lv_obj_t *screen = lv_display_get_screen_active(display);

        lv_obj_clean(screen);
        release_lvgl();
    }
}

void Graphics::background(lv_display_t *display, lv_color_t color)
{
    if (seize_lvgl())
    {
        lv_obj_t *screen = lv_display_get_screen_active(display);

        lv_obj_set_style_bg_color(screen, color, LV_PART_MAIN);
        release_lvgl();
    }
}

void Graphics::logo(lv_display_t *display, const void *src)
{
    if (seize_lvgl())
    {
        lv_obj_t *screen = lv_display_get_screen_active(display);

        lv_obj_t *logo = lv_image_create(screen);
        lv_image_set_src(logo, src);
        lv_obj_align(logo, LV_ALIGN_LEFT_MID, 0, 0);

        release_lvgl();
    }
}

lv_obj_t *Graphics::message(lv_display_t *display, lv_style_t *style, const char *message, lv_label_long_mode_t long_mode)
{
    lv_obj_t *label = nullptr;

    if (seize_lvgl())
    {
        lv_obj_t *screen = lv_display_get_screen_active(display);

        label = lv_label_create(screen);
        if (style != nullptr) {
            lv_obj_add_style(label, style, LV_STATE_DEFAULT);
        }
        lv_label_set_long_mode(label, long_mode);
        lv_label_set_text(label, message);

        release_lvgl();
    }
    return label;
}

void Graphics::qrcode(lv_display_t *display, const char *data, lv_color_t light_color, lv_color_t dark_color)
{
#ifdef CONFIG_LV_USE_QRCODE
    if (seize_lvgl())
    {
        lv_obj_t *screen = lv_display_get_screen_active(display);
        int32_t height = lv_display_get_vertical_resolution(display);

        lv_obj_t *qr = lv_qrcode_create(screen);
        lv_qrcode_set_size(qr, height);
        lv_qrcode_set_dark_color(qr, dark_color);
        lv_qrcode_set_light_color(qr, light_color);

        lv_qrcode_update(qr, data, strlen(data));
        lv_obj_center(qr);
        lv_obj_align(qr, LV_ALIGN_LEFT_MID, 0, 0);

        release_lvgl();
    }
#else
    ESP_LOGW(TAG, "QR Code support is not enabled in LVGL configuration.");
#endif
}

void Graphics::cross(lv_display_t *display, lv_style_t *style)
{
    if (seize_lvgl())
    {
        lv_obj_t *screen = lv_display_get_screen_active(display);

        int32_t width = lv_display_get_horizontal_resolution(display);
        int32_t height = lv_display_get_vertical_resolution(display);

        lv_point_precise_t line_1_points[] = { {0, 0}, {width, height} };
        lv_point_precise_t line_2_points[] = { {0, height}, {width, 0} };

        lv_obj_t *line1 = lv_line_create(screen);
        lv_line_set_points(line1, line_1_points, 2);
        if (style != nullptr) {
            lv_obj_add_style(line1, style, LV_STATE_DEFAULT);
        }

        lv_obj_t *line2 = lv_line_create(screen);
        lv_line_set_points(line2, line_2_points, 2);
        if (style != nullptr) {
            lv_obj_add_style(line2, style, LV_STATE_DEFAULT);
        }

        release_lvgl();
    }
}

void Graphics::spinner(lv_display_t *display, int32_t size, lv_style_t *style)
{
    if (seize_lvgl())
    {
        lv_obj_t *screen = lv_display_get_screen_active(display);

        lv_obj_t *spinner = lv_spinner_create(screen);
        if (style != nullptr) {
            lv_obj_add_style(spinner, style, LV_STATE_DEFAULT);
        }
        lv_obj_set_size(spinner, size, size);

        release_lvgl();
    }
}

lv_obj_t *Graphics::led(lv_display_t *display, int32_t size, lv_color_t color)
{
    lv_obj_t *led = nullptr;

    if (seize_lvgl())
    {
        lv_obj_t *screen = lv_display_get_screen_active(display);

        led = lv_led_create(screen);
        lv_obj_set_size(led, size, size);
        lv_obj_align(led, LV_ALIGN_CENTER, 0, 0);
        lv_led_set_color(led, color);
        lv_led_off(led);

        release_lvgl();
    }
    return led;
}
