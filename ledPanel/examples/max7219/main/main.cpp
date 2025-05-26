#include <stdio.h>
#include <esp_log.h>
#include <ledPanel.hpp>

static const char *TAG = "max7219";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "LED Panel Test Program");

    const lvgl_port_cfg_t lvglConfig = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvglConfig);

    macdap::LedPanel &ledPanel = macdap::LedPanel::getInstance();
    // ledPanel.setBrightness(1.00);
    // ledPanel.scrollingMessage("Hello World from MacDap");
    ledPanel.message("12345678");

    ESP_LOGI(TAG, "LED Panel Test Program");
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
