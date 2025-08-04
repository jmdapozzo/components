#include <ledPanel.hpp>
#include <esp_log.h>
#include <logo.h>
#include <font.h>

static const char *TAG = "mbi5026";
static lv_obj_t *_scr;
lv_coord_t _width;
lv_coord_t _height;

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
        lv_img_set_src(logo, &bwLogoNoText16x16);
        lv_obj_align(logo, LV_ALIGN_LEFT_MID, 0, 0);

        lvgl_port_unlock();
    }
}

void greeting(const char *projectName, const char *version)
{
    if (lvgl_port_lock(0)) 
    {
        lv_obj_set_style_text_font(_scr, &nothingFont5x7, 0);

        lv_obj_t *labelProjectName = lv_label_create(_scr);
        lv_obj_align(labelProjectName, LV_ALIGN_TOP_MID, 0, 0);
        lv_label_set_text(labelProjectName, projectName);

        lv_obj_t * labelVersion = lv_label_create(_scr);
        lv_obj_align(labelVersion, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_label_set_text(labelVersion, version);

        lvgl_port_unlock();
    }
}

void message(const char *message)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_set_style_text_font(_scr, &lv_font_unscii_8, 0);

        lv_obj_t *label = lv_label_create(_scr);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(label, message);

        lvgl_port_unlock();
    }
}

void scrollingMessage(const char *message)
{

    if (lvgl_port_lock(0))
    {
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
        
        lv_obj_t *label = lv_label_create(_scr);
        lv_obj_add_style(label, &style, LV_STATE_DEFAULT);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(label, _width);
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
    _scr = lv_disp_get_scr_act(display);
    _width = lv_disp_get_hor_res(display);
    _height = lv_disp_get_ver_res(display);
    background(LV_PALETTE_NONE);
    ESP_LOGI(TAG, "Display resolution: %ld x %ld", _width, _height);

    float brightness = 1.0;
    ledPanel.setBrightness(brightness);

    logo();
    vTaskDelay(pdMS_TO_TICKS(3000));
    clear();

    greeting("mbi5026", "0.0.0");
    vTaskDelay(pdMS_TO_TICKS(3000));
    clear();

    message("MacDap");
    vTaskDelay(pdMS_TO_TICKS(3000));
    clear();

    scrollingMessage("Test program x64y32, Version 0.0.0 from MacDap Inc.");

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
