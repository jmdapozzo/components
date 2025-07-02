#include "ledMatrix.hpp"
#include <esp_log.h>
#include <logo.h>

static const char *TAG = "x64y32";
static lv_obj_t *scr;
lv_coord_t _width;
lv_coord_t _height;

void clear()
{
    if (lvgl_port_lock(0))
    {
        lv_obj_clean(scr);
        lvgl_port_unlock();
    }
}

void background(lv_palette_t color)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_set_style_bg_color(scr,lv_palette_main(color),LV_PART_MAIN);
        lvgl_port_unlock();
    }
}

void logo(void)
{
    static lv_obj_t *logo;

    if (lvgl_port_lock(0)) 
    {
        logo = lv_img_create(scr);
        lv_img_set_src(logo, &colorLogo32x32);
        lv_obj_align(logo, LV_ALIGN_LEFT_MID, 0, 0);

        lvgl_port_unlock();
    }
}

void greeting(const char *projectName, const char *version)
{
    static lv_obj_t *logo;
    static lv_obj_t *labelProjectName;

    if (lvgl_port_lock(0)) 
    {
        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_text_font(&style, &lv_font_montserrat_14);
        lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_AMBER));
        lv_style_set_align(&style, LV_ALIGN_CENTER);

        labelProjectName = lv_label_create(scr);
        lv_obj_add_style(labelProjectName, &style, 0);
        lv_label_set_text(labelProjectName, projectName);
        lv_obj_align(labelProjectName, LV_ALIGN_TOP_LEFT, 1, 0);

        lv_obj_t * labelVersion = lv_label_create(scr);
        lv_obj_add_style(labelVersion, &style, 0);
        lv_label_set_text(labelVersion, version);
        lv_obj_align(labelVersion, LV_ALIGN_BOTTOM_LEFT, -1, 0);

        lvgl_port_unlock();
    }
}

void message(const char *message)
{
    static lv_obj_t *label = nullptr;

    if (lvgl_port_lock(0))
    {
        // lv_obj_set_style_bg_color(scr,lv_palette_main(LV_PALETTE_NONE),LV_PART_MAIN);

        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_text_font(&style, &lv_font_montserrat_20);
        lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_GREEN));
        lv_style_set_align(&style, LV_ALIGN_CENTER);

        label = lv_label_create(scr);
        lv_obj_add_style(label, &style, 0);

        lv_label_set_text(label, message);
        lvgl_port_unlock();
    }
}

void scrollingMessage(const char *message)
{
    static lv_obj_t *label = nullptr;

    if (lvgl_port_lock(0))
    {
      static lv_style_t style;
      lv_style_init(&style);
      lv_style_set_text_font(&style, &lv_font_montserrat_18);
      // lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_NONE));
      lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_GREEN));
      lv_style_set_anim_speed(&style, 10);
      lv_style_set_align(&style, LV_ALIGN_BOTTOM_MID);
      
      label = lv_label_create(scr);
      lv_obj_add_style(label, &style, 0);
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
      lv_style_set_anim_speed(&style, 20);
      lv_style_set_align(&style, LV_ALIGN_TOP_RIGHT);
      
      label = lv_label_create(scr);
      lv_obj_add_style(label, &style, 0);
      lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
      lv_obj_set_width(label, _width);
      lv_label_set_text(label, message);

      lvgl_port_unlock();
    }
}

void qrcode(void)
{
    lv_color_t bg_color = lv_palette_lighten(LV_PALETTE_NONE, 5);
    lv_color_t fg_color = lv_palette_darken(LV_PALETTE_AMBER, 4);

    lv_obj_t * qr = lv_qrcode_create(scr, 32, fg_color, bg_color);

    const char * data = "https://macdap.net";
    lv_qrcode_update(qr, data, strlen(data));
    lv_obj_center(qr);
    lv_obj_align(qr, LV_ALIGN_LEFT_MID, 0, 0);
}

lv_obj_t *led(void)
{
    lv_obj_t *led  = lv_led_create(scr);
    lv_obj_set_size(led, 8, 8);
    lv_obj_align(led, LV_ALIGN_TOP_RIGHT, -8, 4);
    lv_led_set_color(led, lv_palette_main(LV_PALETTE_RED));
    lv_led_off(led);
    return led;
}

extern "C" void app_main(void)
{
  ESP_LOGI(TAG, "LED Matrix Test Program");

  const lvgl_port_cfg_t lvglConfig = ESP_LVGL_PORT_INIT_CONFIG();
  lvgl_port_init(&lvglConfig);

  macdap::LedMatrix &ledMatrix = macdap::LedMatrix::getInstance();
  lv_disp_t *display = ledMatrix.getLvDisp();
  scr = lv_disp_get_scr_act(display);
  _width = lv_disp_get_hor_res(display);
  _height = lv_disp_get_ver_res(display);
  background(LV_PALETTE_NONE);
  ESP_LOGI(TAG, "Display resolution: %d x %d", _width, _height);

  float brightness = 100.0;
  ledMatrix.setBrightness(brightness);

  logo();
  vTaskDelay(pdMS_TO_TICKS(3000));
  clear();

  greeting("x64y32", "Version 0.0.0");
  vTaskDelay(pdMS_TO_TICKS(3000));
  clear();

  message("MacDap Inc.");
  vTaskDelay(pdMS_TO_TICKS(3000));
  clear();

  brightness = 10.0;
  ledMatrix.setBrightness(brightness);

  qrcode();
  vTaskDelay(pdMS_TO_TICKS(3000));
  clear();

  brightness = 1.0;
  ledMatrix.setBrightness(brightness);
  lv_obj_t *heartbeat = led();
  scrollingMessage("Test program x64y32, Version 0.0.0 from MacDap Inc.");
  scrollingMessage2("MacDap Inc. the best");

  // float step = -10.0f;
  // static int palette_idx = 0;
  // lv_palette_t palettes[] = {
  //   LV_PALETTE_RED, LV_PALETTE_PINK, LV_PALETTE_PURPLE, LV_PALETTE_DEEP_PURPLE,
  //   LV_PALETTE_INDIGO, LV_PALETTE_BLUE, LV_PALETTE_LIGHT_BLUE, LV_PALETTE_CYAN,
  //   LV_PALETTE_TEAL, LV_PALETTE_GREEN, LV_PALETTE_LIGHT_GREEN, LV_PALETTE_LIME,
  //   LV_PALETTE_YELLOW, LV_PALETTE_AMBER, LV_PALETTE_ORANGE, LV_PALETTE_DEEP_ORANGE,
  //   LV_PALETTE_BROWN, LV_PALETTE_GREY, LV_PALETTE_BLUE_GREY
  // };
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
    // ESP_LOGI(TAG, "Heartbeat LED toggled");

    // int num_palettes = sizeof(palettes) / sizeof(palettes[0]);
    // background(palettes[palette_idx]);
    // palette_idx = (palette_idx + 1) % num_palettes;

    // ledMatrix.setBrightness(brightness);
    // brightness += step;
    // if (brightness <= 10.0f) {
    //   brightness = 10.0f;
    //   step = 10.0f;
    // } else if (brightness >= 100.0f) {
    //   brightness = 100.0f;
    //   step = -10.0f;
    // }
  }
}
