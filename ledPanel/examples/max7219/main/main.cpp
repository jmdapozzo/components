#include <stdio.h>
#include <esp_log.h>
#include <ledPanel.hpp>

static const char *TAG = "max7219";

void message(const char *message)
{
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "LED Panel Test Program");

    const lvgl_port_cfg_t lvglConfig = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvglConfig);

    macdap::LedPanel &ledPanel = macdap::LedPanel::get_instance();
    ledPanel.set_brightness(1.00);
    message("12345678");
    // ledPanel.message("1234");
    // ledPanel.message("-.-");

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


