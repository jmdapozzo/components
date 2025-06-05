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
    // ledPanel.message("1234");
    // ledPanel.message("-.-");
    // ledPanel.test();

    ESP_LOGI(TAG, "LED Panel Test Program");
    uint8_t brightness = 0;
    while (true)
    {
        // ledPanel.setBrightness(brightness);
        vTaskDelay(pdMS_TO_TICKS(1000));
        brightness = (brightness + 10) % 100;
    }
}
