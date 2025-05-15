#include <display.hpp>
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/i2c_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <lvgl.h>
#include <esp_lcd_panel_dev.h>
#include <esp_lcd_panel_ssd1306.h>
#include <logo.h>

#if CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
#include "esp_lcd_sh1107.h"
#else
#include "esp_lcd_panel_vendor.h"
#endif

using namespace macdap;

#define LCD_PIXEL_CLOCK_HZ    (400 * 1000)

#if CONFIG_LCD_CONTROLLER_SSD1306
#define DISP_WIDTH              128
#define DISP_HEIGHT             CONFIG_SSD1306_HEIGHT
#elif CONFIG_LCD_CONTROLLER_SH1107
#define DISP_WIDTH              64
#define DISP_HEIGHT             128
#endif

#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8

static const char *TAG = "display";

Display::Display()
{
    ESP_LOGI(TAG, "Initializing...");

    i2c_master_bus_handle_t i2cMasterBusHandle;
    ESP_ERROR_CHECK(i2c_master_get_bus_handle(0, &i2cMasterBusHandle));
    if (i2c_master_probe(i2cMasterBusHandle, CONFIG_I2C_LCD_CONTROLLER_ADDR, 100) != ESP_OK)
    {
        ESP_LOGW(TAG, "I2C device not found");
        return;
    }

    esp_lcd_panel_io_i2c_config_t ioConfig;
    ioConfig.dev_addr = CONFIG_I2C_LCD_CONTROLLER_ADDR;
    ioConfig.scl_speed_hz = LCD_PIXEL_CLOCK_HZ;
    ioConfig.control_phase_bytes = 1;               // According to SSD1306 datasheet
    ioConfig.lcd_cmd_bits = LCD_CMD_BITS;           // According to SSD1306 datasheet
    ioConfig.lcd_param_bits = LCD_CMD_BITS;         // According to SSD1306 datasheet
#if CONFIG_LCD_CONTROLLER_SSD1306
    ioConfig.dc_bit_offset = 6;                     // According to SSD1306 datasheet
#elif CONFIG_LCD_CONTROLLER_SH1107
    ioConfig.dc_bit_offset = 0;                     // According to SH1107 datasheet
    ioConfig.flags =
    {
        .disable_control_phase = 1
    }
#endif

    esp_lcd_panel_io_handle_t ioHandle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2cMasterBusHandle, &ioConfig, &ioHandle));

    esp_lcd_panel_dev_config_t panelConfig;
    panelConfig.bits_per_pixel = 1;
    panelConfig.reset_gpio_num = CONFIG_GPIO_RESET;

#if CONFIG_LCD_CONTROLLER_SSD1306
    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    esp_lcd_panel_ssd1306_config_t ssd1306Config;
    ssd1306Config.height = DISP_HEIGHT;

    esp_lcd_panel_handle_t panelHandle = NULL;

    panelConfig.vendor_config = &ssd1306Config;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(ioHandle, &panelConfig, &panelHandle));
#elif CONFIG_LCD_CONTROLLER_SH1107
    ESP_LOGI(TAG, "Install SH1107 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_sh1107(io_handle, &panelConfig, &panelHandle));
#endif

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panelHandle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panelHandle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panelHandle, true));

#if CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panelHandle, true));
#endif

    // const lvgl_port_cfg_t lvglConfig = ESP_LVGL_PORT_INIT_CONFIG();
    // lvgl_port_init(&lvglConfig);

    const lvgl_port_display_cfg_t dispConfig = {
        .io_handle = ioHandle,
        .panel_handle = panelHandle,
        .buffer_size = DISP_WIDTH * DISP_HEIGHT,
        .double_buffer = true,
        .hres = DISP_WIDTH,
        .vres = DISP_HEIGHT,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        }
    };

    m_lvDisp = lvgl_port_add_disp(&dispConfig); // Warning "Callback on_color_trans_done was already set and now it was overwritten!" comes from this call.

    /* Rotation of the screen */
    lv_disp_set_rotation(m_lvDisp, LV_DISP_ROT_NONE);
}

Display::~Display()
{
}

void Display::clear()
{
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (m_lvDisp && lvgl_port_lock(0)) {
        lv_obj_t *scr = lv_disp_get_scr_act(m_lvDisp);
        lv_obj_clean(scr);
        lvgl_port_unlock();
    }
}

static void anim_x_cb(void * var, int32_t v)
{
    lv_obj_set_x((_lv_obj_t*)var, v);
}

static void anim_size_cb(void * var, int32_t v)
{
    lv_obj_set_size((_lv_obj_t*)var, v, v);
}


void Display::greeting(const char *projectName, const char *version)
{
    ESP_LOGI(TAG, "Display LVGL Scroll Text");
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (m_lvDisp && lvgl_port_lock(0)) {
        lv_obj_t *scr = lv_disp_get_scr_act(m_lvDisp);

        lv_obj_t *logo = lv_img_create(scr);
        lv_img_set_src(logo, &logo48x48);
        lv_obj_align(logo, LV_ALIGN_TOP_LEFT, 10, 0);

        lv_obj_t * labelProjectName = lv_label_create(scr);
        //lv_label_set_long_mode(labelProjectName, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_label_set_text(labelProjectName, projectName);
        //lv_obj_set_width(labelProjectName,70);
        lv_obj_align(labelProjectName, LV_ALIGN_LEFT_MID, 60, 0);

        lv_obj_t * labelVersion = lv_label_create(scr);
        //lv_label_set_long_mode(labelVersion, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_label_set_text(labelVersion, version);
        //lv_obj_set_width(labelVersion, m_lvDisp->driver->hor_res);
        lv_obj_align(labelVersion, LV_ALIGN_BOTTOM_LEFT, 0, 0);

        // lv_obj_t *labelTop = lv_label_create(scr);
        // lv_label_set_long_mode(labelTop, LV_LABEL_LONG_SCROLL_CIRCULAR); /* Circular scroll */
        // lv_label_set_text(labelTop, "Hello, World! This is a long text to test the circular scrolling feature.");
        // lv_obj_set_width(labelTop, m_lvDisp->driver->hor_res);
        // lv_obj_align(labelTop, LV_ALIGN_TOP_MID, 0, 0);

        // lv_obj_t * imgLogo = lv_img_create(scr);
        // lv_img_set_src(imgLogo, &macdapLogoVertical);
        // lv_obj_align(imgLogo, LV_ALIGN_CENTER, 0, -20);
        // lv_obj_set_size(imgLogo, 64, 64);

        // lv_obj_t *labelBottom = lv_label_create(scr);
        // lv_label_set_long_mode(labelBottom, LV_LABEL_LONG_SCROLL_CIRCULAR); /* Circular scroll */
        // lv_label_set_text_fmt(labelBottom, "%s %s", projectName, version);
        // lv_obj_set_width(labelBottom, m_lvDisp->driver->hor_res);
        // lv_obj_align(labelBottom, LV_ALIGN_BOTTOM_MID, 0, 0);

        lvgl_port_unlock();
    }
}

void Display::message(const char *message)
{
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (m_lvDisp && lvgl_port_lock(0)) {
        lv_obj_t *scr = lv_disp_get_scr_act(m_lvDisp);

        lv_obj_t * label = lv_label_create(scr);
        lv_label_set_text(label, message);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        //lv_label_set_text(label1, LV_SYMBOL_HOME " Goodby ");

        lvgl_port_unlock();
    }
}

