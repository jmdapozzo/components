#include <stdio.h>
#include <esp_log.h>
#include <ledPanel.hpp>

static const char *TAG = "mbi5026";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "LED Panel Test Program");

    const lvgl_port_cfg_t lvglConfig = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvglConfig);

    macdap::LedPanel &ledPanel = macdap::LedPanel::getInstance();
    ledPanel.setBrightness(1.0);
    // ledPanel.test();
    // ledPanel.message("1234");
    ledPanel.scrollingMessage("Hello World from MacDap");

    ESP_LOGI(TAG, "LED Panel Test Program");
    float brightness = 0.0;
    while (true)
    {
        // ledPanel.setBrightness(brightness);
        vTaskDelay(pdMS_TO_TICKS(1000));
        brightness += 10.0;
        if (brightness > 100.0)
            brightness = 0;
    }
}
