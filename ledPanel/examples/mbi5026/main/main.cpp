#include <ledPanel.hpp>
#include <esp_log.h>
#include <logo.h>
#include <font.h>

static const char *TAG = "mbi5026";

void clear(lv_display_t *display)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);
        lv_obj_clean(scr);
        lvgl_port_unlock();
    }
}

void background(lv_display_t *display, lv_palette_t color)
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
        lv_image_set_src(logo, &bwLogoNoText16x16);
        lv_obj_align(logo, LV_ALIGN_LEFT_MID, 0, 0);

        lvgl_port_unlock();
    }
}

void greeting(lv_display_t *display, const char *projectName, const char *version)
{
    if (lvgl_port_lock(0)) 
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);

        lv_obj_set_style_text_font(scr, &nothingFont5x7, 0);

        lv_obj_t *labelProjectName = lv_label_create(scr);
        lv_obj_align(labelProjectName, LV_ALIGN_TOP_MID, 0, 0);
        lv_label_set_text(labelProjectName, projectName);

        lv_obj_t * labelVersion = lv_label_create(scr);
        lv_obj_align(labelVersion, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_label_set_text(labelVersion, version);

        lvgl_port_unlock();
    }
}

void message(lv_display_t *display, const char *message)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_t *scr = lv_display_get_screen_active(display);

        lv_obj_set_style_text_font(scr, &lv_font_unscii_8, 0);

        lv_obj_t *label = lv_label_create(scr);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(label, message);

        lvgl_port_unlock();
    }
}

void scrollingMessage(lv_display_t *display, const char *message)
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
        lv_style_set_text_font(&style, &lv_font_unscii_8);
        lv_style_set_align(&style, LV_ALIGN_CENTER);
        lv_style_set_anim(&style, &animationTemplate);
        
        lv_obj_t *label = lv_label_create(scr);
        lv_obj_add_style(label, &style, LV_STATE_DEFAULT);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(label, width);
        lv_label_set_text(label, message);

        lvgl_port_unlock();
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "LED Panel Test Program");

    const lvgl_port_cfg_t lvglConfig = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvglConfig);

    macdap::LedPanel &ledPanel = macdap::LedPanel::getInstance();
    lv_display_t *display = ledPanel.getLvDisplay();

    background(display, LV_PALETTE_NONE);

    float brightness = 1.0;
    ledPanel.setBrightness(brightness);

    logo(display);
    vTaskDelay(pdMS_TO_TICKS(3000));
    clear(display);

    greeting(display, "mbi5026", "0.0.0");
    vTaskDelay(pdMS_TO_TICKS(3000));
    clear(display);

    message(display, "MacDap");
    vTaskDelay(pdMS_TO_TICKS(3000));
    clear(display);

    scrollingMessage(display, "Test program x64y32, Version 0.0.0 from MacDap Inc.");

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
