#include <stdio.h>
#include <esp_log.h>
#include <ledPanel.hpp>

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "LED Panel Test Program");

    const lvgl_port_cfg_t lvglConfig = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvglConfig);

    macdap::LedPanel &ledPanel = macdap::LedPanel::getInstance();
    ledPanel.setBrightness(1.00);
    ledPanel.message("-.-");
    //ledPanel.scrollingMessage("-.-\no.o\nx.x\n\n");
    //ledPanel.scrollingMessage("Hello World from MacDap");

    while (true)
    {
        ESP_LOGI(TAG, "LED Panel Test Program");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
