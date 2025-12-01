#include <screenLayout.hpp>
#include <esp_log.h>
#include <esp_lvgl_port.h>
#include <icons.h>

extern const lv_image_dsc_t empty_16;
extern const lv_image_dsc_t empty_32;

using namespace macdap;

static const char *TAG = "screenLayout";

ScreenLayout::ScreenLayout(lv_display_t *display, 
                          lv_style_t *header_style,
                          const char *header_text, 
                          lv_style_t *footer_style,
                          const char *footer_text,
                          int32_t header_h, 
                          int32_t footer_h)
    : screen(nullptr)
    , header(nullptr)
    , left_grp(nullptr)
    , header_label(nullptr)
    , right_grp(nullptr)
    , main_area(nullptr)
    , footer(nullptr)
    , footer_label(nullptr)
    , header_height(header_h)
    , footer_height(footer_h)
{
    if (!display)
    {
        ESP_LOGE(TAG, "Display is null");
        return;
    }

    screen_width = lv_display_get_horizontal_resolution(display);
    screen_height = lv_display_get_vertical_resolution(display);

    ESP_LOGI(TAG, "Creating layout: %dx%d, header=%d, footer=%d", 
             screen_width, screen_height, header_height, footer_height);

    if (lvgl_port_lock(0))
    {
        screen = lv_display_get_screen_active(display);

        lv_obj_clean(screen);

        lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);

        // Header: fixed height, 3 sections (left icons, center title, right icons)
        header = lv_obj_create(screen);
        lv_obj_set_size(header, screen_width, header_height);
        lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_all(header, 0, 0);
        lv_obj_set_style_pad_gap(header, 2, 0);
        lv_obj_set_style_bg_opa(header, LV_OPA_20, 0);
        lv_obj_set_style_border_width(header, 0, 0);
        lv_obj_set_flex_align(header,
                            LV_FLEX_ALIGN_SPACE_BETWEEN,
                            LV_FLEX_ALIGN_CENTER,
                            LV_FLEX_ALIGN_CENTER);

        // Left icon group
        left_grp = lv_obj_create(header);
        lv_obj_set_size(left_grp, LV_SIZE_CONTENT, header_height);
        lv_obj_set_flex_flow(left_grp, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_all(left_grp, 0, 0);
        lv_obj_set_style_pad_gap(left_grp, 2, 0);
        lv_obj_set_style_bg_opa(left_grp, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(left_grp, 0, 0);

        // Center title (will take remaining space)
        header_label = lv_label_create(header);
        lv_label_set_text(header_label, header_text);
        lv_obj_set_flex_grow(header_label, 1);
        if (header_style != nullptr) {
            lv_obj_add_style(header_label, header_style, LV_STATE_DEFAULT);
        } else {
            lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_style_text_font(header_label, &lv_font_montserrat_12, 0);
        }

        // Right icon group
        right_grp = lv_obj_create(header);
        lv_obj_set_size(right_grp, LV_SIZE_CONTENT, header_height);
        lv_obj_set_flex_flow(right_grp, LV_FLEX_FLOW_ROW_REVERSE);
        lv_obj_set_style_pad_all(right_grp, 0, 0);
        lv_obj_set_style_pad_gap(right_grp, 2, 0);
        lv_obj_set_style_bg_opa(right_grp, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(right_grp, 0, 0);

        // Main content area (grows to fill available space)
        main_area = lv_obj_create(screen);
        lv_obj_set_size(main_area, screen_width, LV_SIZE_CONTENT);
        lv_obj_set_flex_grow(main_area, 1);
        lv_obj_set_style_pad_all(main_area, 0, 0);
        lv_obj_set_style_bg_opa(main_area, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(main_area, 0, 0);

        // Footer: fixed height, full-width text
        footer = lv_obj_create(screen);
        lv_obj_set_size(footer, screen_width, footer_height);
        lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_all(footer, 0, 0);
        lv_obj_set_style_border_width(footer, 0, 0);
        lv_obj_set_flex_align(footer,
                            LV_FLEX_ALIGN_CENTER,
                            LV_FLEX_ALIGN_CENTER,
                            LV_FLEX_ALIGN_CENTER);

        footer_label = lv_label_create(footer);
        lv_label_set_text(footer_label, footer_text);
        lv_obj_set_width(footer_label, screen_width);
        if (footer_style != nullptr) {
            lv_obj_add_style(footer_label, footer_style, LV_STATE_DEFAULT);
        } else {
            lv_obj_set_style_text_align(footer_label, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_style_text_font(footer_label, &lv_font_montserrat_12, 0);
        }

        lvgl_port_unlock();
    }
}

ScreenLayout::~ScreenLayout()
{
    // LVGL objects are managed by the display, no manual cleanup needed
    left_icons.clear();
    right_icons.clear();
}

lv_obj_t *ScreenLayout::add_left_icon(const lv_image_dsc_t *icon_src)
{
    if (!left_grp || !icon_src) return nullptr;

    lv_obj_t *icon = nullptr;
    if (lvgl_port_lock(0))
    {
        icon = lv_img_create(left_grp);
        lv_img_set_src(icon, icon_src);
        left_icons.push_back(icon);
        lvgl_port_unlock();
    }
    return icon;
}

lv_obj_t *ScreenLayout::add_left_icon()
{
    switch (header_height)
    {
        case 32:
            return add_left_icon(&empty_32);
        break;

        case 16:
            return add_left_icon(&empty_16);
        break;

        default:
            ESP_LOGE(TAG, "Unexpected icon height: %d", header_height);
            return nullptr;
        break;
    }
}

lv_obj_t *ScreenLayout::add_right_icon(const lv_image_dsc_t *icon_src)
{
    if (!right_grp || !icon_src) return nullptr;

    lv_obj_t *icon = nullptr;
    if (lvgl_port_lock(0))
    {
        icon = lv_img_create(right_grp);
        lv_img_set_src(icon, icon_src);
        right_icons.push_back(icon);
        lvgl_port_unlock();
    }
    return icon;
}

lv_obj_t *ScreenLayout::add_right_icon()
{
    switch (header_height)
    {
        case 32:
            return add_right_icon(&empty_32);
        break;

        case 16:
            return add_right_icon(&empty_16);
        break;

        default:
            ESP_LOGE(TAG, "Unexpected icon height: %d", header_height);
            return nullptr;
        break;
    }
}

void ScreenLayout::set_header_text(const char *text)
{
    if (!header_label || !text) return;

    if (lvgl_port_lock(0))
    {
        lv_label_set_text(header_label, text);
        lvgl_port_unlock();
    }
}

lv_obj_t *ScreenLayout::get_header_label()
{
    return header_label;
}

void ScreenLayout::set_footer_text(const char *text)
{
    if (!footer_label || !text) return;

    if (lvgl_port_lock(0))
    {
        lv_label_set_text(footer_label, text);
        lvgl_port_unlock();
    }
}

lv_obj_t *ScreenLayout::get_footer_label()
{
    return footer_label;
}

lv_obj_t *ScreenLayout::get_main_area()
{
    return main_area;
}

lv_obj_t *ScreenLayout::get_header()
{
    return header;
}

lv_obj_t *ScreenLayout::get_footer()
{
    return footer;
}

void ScreenLayout::clear_left_icons()
{
    if (lvgl_port_lock(0))
    {
        for (auto icon : left_icons)
        {
            if (icon) lv_obj_delete(icon);
        }
        left_icons.clear();
        lvgl_port_unlock();
    }
}

void ScreenLayout::clear_right_icons()
{
    if (lvgl_port_lock(0))
    {
        for (auto icon : right_icons)
        {
            if (icon) lv_obj_delete(icon);
        }
        right_icons.clear();
        lvgl_port_unlock();
    }
}
