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

lv_obj_t *Graphics::logo(lv_display_t *display, const void *src, lv_style_t *style)
{
    lv_obj_t *logo = nullptr;

    if (seize_lvgl())
    {
        lv_obj_t *screen = lv_display_get_screen_active(display);

        logo = lv_image_create(screen);
        lv_image_set_src(logo, src);
        if (style != nullptr) {
            lv_obj_add_style(logo, style, LV_STATE_DEFAULT);
        }

        release_lvgl();
    }
    return logo;
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

void Graphics::dot(lv_display_t *display, lv_style_t *style, int32_t x, int32_t y)
{
    if (seize_lvgl())
    {
        lv_obj_t *screen = lv_display_get_screen_active(display);

        lv_obj_t *dot = lv_obj_create(screen);
        lv_obj_set_size(dot, x, y);
        if (style != nullptr) {
            lv_obj_add_style(dot, style, LV_STATE_DEFAULT);
        }

        release_lvgl();
    }
}

void Graphics::horizontal(lv_display_t *display, lv_style_t *style)
{
    if (seize_lvgl())
    {
        lv_obj_t *screen = lv_display_get_screen_active(display);

        int32_t width = lv_display_get_horizontal_resolution(display);
        int32_t height = lv_display_get_vertical_resolution(display);

        lv_point_precise_t horizontal_line_points[] = { {0, height/2}, {width, height/2} };

        lv_obj_t *horizontal_line = lv_line_create(screen);
        lv_line_set_points(horizontal_line, horizontal_line_points, 2);
        if (style != nullptr) {
            lv_obj_add_style(horizontal_line, style, LV_STATE_DEFAULT);
        }

        release_lvgl();
    }
}
void Graphics::vertical(lv_display_t *display, lv_style_t *style)
{
    if (seize_lvgl())
    {
        lv_obj_t *screen = lv_display_get_screen_active(display);

        int32_t width = lv_display_get_horizontal_resolution(display);
        int32_t height = lv_display_get_vertical_resolution(display);

        lv_point_precise_t vertical_line_points[] = { {width/2, 0}, {width/2, height} };

        lv_obj_t *vertical_line = lv_line_create(screen);
        lv_line_set_points(vertical_line, vertical_line_points, 2);
        if (style != nullptr) {
            lv_obj_add_style(vertical_line, style, LV_STATE_DEFAULT);
        }

        release_lvgl();
    }
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

void Graphics::delete_widget(lv_obj_t *widget)
{
    if (widget != nullptr)
    {
        if (seize_lvgl())
        {
            lv_obj_delete(widget);
            release_lvgl();
        }
    }
}

lv_obj_t *Graphics::create_message(lv_display_t *display, const char *message, lv_style_t *style, lv_label_long_mode_t long_mode)
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

bool Graphics::update_message(lv_obj_t *message_widget, const char *message)
{
    if (message_widget == nullptr || message == nullptr) return false;

    if (seize_lvgl())
    {
        lv_label_set_text(message_widget, message);
        release_lvgl();
        return true;
    }
    return false;
}

lv_obj_t *Graphics::create_wifi_status_icon(lv_display_t *display, WifiStatus status, IconSize size, int32_t x, int32_t y)
{
    const lv_image_dsc_t *icon_src = nullptr;
    
    if (size == IconSize::SIZE_32)
    {
        switch (status)
        {
            case WifiStatus::NONE:                  icon_src = &wifi_none_32; break;
            case WifiStatus::LOW:                   icon_src = &wifi_low_32; break;
            case WifiStatus::MEDIUM:                icon_src = &wifi_medium_32; break;
            case WifiStatus::HIGH:                  icon_src = &wifi_high_32; break;
            case WifiStatus::DISCONNECTED_SLASH:    icon_src = &wifi_slash_32; break;
            case WifiStatus::DISCONNECTED_X:        icon_src = &wifi_x_32; break;
            case WifiStatus::BROADCAST:             icon_src = &broadcast_32; break;
            default:                                icon_src = &wifi_none_32; break;
        }
    }
    else
    {
        switch (status)
        {
            case WifiStatus::NONE:                  icon_src = &wifi_none_16; break;
            case WifiStatus::LOW:                   icon_src = &wifi_low_16; break;
            case WifiStatus::MEDIUM:                icon_src = &wifi_medium_16; break;
            case WifiStatus::HIGH:                  icon_src = &wifi_high_16; break;
            case WifiStatus::DISCONNECTED_SLASH:    icon_src = &wifi_slash_16; break;
            case WifiStatus::DISCONNECTED_X:        icon_src = &wifi_x_16; break;
            case WifiStatus::BROADCAST:             icon_src = &broadcast_16; break;
            default:                                icon_src = &wifi_none_16; break;
        }
    }
    
    if (seize_lvgl())
    {
        lv_obj_t *icon = lv_img_create(lv_display_get_screen_active(display));
        lv_img_set_src(icon, icon_src);
        lv_obj_set_pos(icon, x, y);
        release_lvgl();
        return icon;
    }
    return nullptr;
}

bool Graphics::update_wifi_status_icon(lv_obj_t *icon_widget, WifiStatus status, IconSize size)
{
    if (!icon_widget) return false;
    
    const lv_image_dsc_t *icon_src = nullptr;
    
    if (size == IconSize::SIZE_32)
    {
        switch (status)
        {
            case WifiStatus::NONE:                  icon_src = &wifi_none_32; break;
            case WifiStatus::LOW:                   icon_src = &wifi_low_32; break;
            case WifiStatus::MEDIUM:                icon_src = &wifi_medium_32; break;
            case WifiStatus::HIGH:                  icon_src = &wifi_high_32; break;
            case WifiStatus::DISCONNECTED_SLASH:    icon_src = &wifi_slash_32; break;
            case WifiStatus::DISCONNECTED_X:        icon_src = &wifi_x_32; break;
            case WifiStatus::BROADCAST:             icon_src = &broadcast_32; break;
            default:                                icon_src = &wifi_none_32; break;
        }
    }
    else
    {
        switch (status)
        {
            case WifiStatus::NONE:                  icon_src = &wifi_none_16; break;
            case WifiStatus::LOW:                   icon_src = &wifi_low_16; break;
            case WifiStatus::MEDIUM:                icon_src = &wifi_medium_16; break;
            case WifiStatus::HIGH:                  icon_src = &wifi_high_16; break;
            case WifiStatus::DISCONNECTED_SLASH:    icon_src = &wifi_slash_16; break;
            case WifiStatus::DISCONNECTED_X:        icon_src = &wifi_x_16; break;
            case WifiStatus::BROADCAST:             icon_src = &broadcast_16; break;
            default:                                icon_src = &wifi_none_16; break;
        }
    }
    
    if (seize_lvgl())
    {
        lv_img_set_src(icon_widget, icon_src);
        release_lvgl();
        return true;
    }
    return false;
}

lv_obj_t *Graphics::create_cellular_status_icon(lv_display_t *display, CellularStatus status, IconSize size, int32_t x, int32_t y)
{
    const lv_image_dsc_t *icon_src = nullptr;
    
    if (size == IconSize::SIZE_32)
    {
        switch (status)
        {
            case CellularStatus::NONE:                  icon_src = &cell_signal_none_32; break;
            case CellularStatus::LOW:                   icon_src = &cell_signal_low_32; break;
            case CellularStatus::MEDIUM:                icon_src = &cell_signal_medium_32; break;
            case CellularStatus::HIGH:                  icon_src = &cell_signal_high_32; break;
            case CellularStatus::FULL:                  icon_src = &cell_signal_full_32; break;
            case CellularStatus::DISCONNECTED_SLASH:    icon_src = &cell_signal_slash_32; break;
            case CellularStatus::DISCONNECTED_X:        icon_src = &cell_signal_x_32; break;
            default:                                    icon_src = &cell_signal_none_32; break;
        }
    }
    else
    {
        switch (status)
        {
            case CellularStatus::NONE:                  icon_src = &cell_signal_none_16; break;
            case CellularStatus::LOW:                   icon_src = &cell_signal_low_16; break;
            case CellularStatus::MEDIUM:                icon_src = &cell_signal_medium_16; break;
            case CellularStatus::HIGH:                  icon_src = &cell_signal_high_16; break;
            case CellularStatus::FULL:                  icon_src = &cell_signal_full_16; break;
            case CellularStatus::DISCONNECTED_SLASH:    icon_src = &cell_signal_slash_16; break;
            case CellularStatus::DISCONNECTED_X:        icon_src = &cell_signal_x_16; break;
            default:                                    icon_src = &cell_signal_none_16; break;
        }
    }
    
    if (seize_lvgl())
    {
        lv_obj_t *icon = lv_img_create(lv_display_get_screen_active(display));
        lv_img_set_src(icon, icon_src);
        lv_obj_set_pos(icon, x, y);
        release_lvgl();
        return icon;
    }
    return nullptr;
}

bool Graphics::update_cellular_status_icon(lv_obj_t *icon_widget, CellularStatus status, IconSize size)
{
    if (!icon_widget) return false;
    
    const lv_image_dsc_t *icon_src = nullptr;
    
    if (size == IconSize::SIZE_32)
    {
        switch (status)
        {
            case CellularStatus::NONE:                  icon_src = &cell_signal_none_32; break;
            case CellularStatus::LOW:                   icon_src = &cell_signal_low_32; break;
            case CellularStatus::MEDIUM:                icon_src = &cell_signal_medium_32; break;
            case CellularStatus::HIGH:                  icon_src = &cell_signal_high_32; break;
            case CellularStatus::FULL:                  icon_src = &cell_signal_full_32; break;
            case CellularStatus::DISCONNECTED_SLASH:    icon_src = &cell_signal_slash_32; break;
            case CellularStatus::DISCONNECTED_X:        icon_src = &cell_signal_x_32; break;
            default:                                    icon_src = &cell_signal_none_32; break;
        }
    }
    else
    {
        switch (status)
        {
            case CellularStatus::NONE:                  icon_src = &cell_signal_none_16; break;
            case CellularStatus::LOW:                   icon_src = &cell_signal_low_16; break;
            case CellularStatus::MEDIUM:                icon_src = &cell_signal_medium_16; break;
            case CellularStatus::HIGH:                  icon_src = &cell_signal_high_16; break;
            case CellularStatus::FULL:                  icon_src = &cell_signal_full_16; break;
            case CellularStatus::DISCONNECTED_SLASH:    icon_src = &cell_signal_slash_16; break;
            case CellularStatus::DISCONNECTED_X:        icon_src = &cell_signal_x_16; break;
            default:                                    icon_src = &cell_signal_none_16; break;
        }
    }
    
    if (seize_lvgl())
    {
        lv_img_set_src(icon_widget, icon_src);
        release_lvgl();
        return true;
    }
    return false;
}

