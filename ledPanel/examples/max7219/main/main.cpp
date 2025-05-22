#include <stdio.h>
#include <esp_log.h>
#include <ledPanel.hpp>

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "LED Panel Test Program");

    const lvgl_port_cfg_t lvglConfig = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvglConfig);

#ifdef CONFIG_LED_PANEL_TYPE_MAX7219
#elif CONFIG_LED_PANEL_TYPE_MBI5026
    macdap::LedPanel ledPanel();
    ledPanel.setBrightness(1.00);
    ledPanel.scrollingMessage("Hello World from MacDap");
#endif    

    while (true)
    {
        ESP_LOGI(TAG, "LED Panel Test Program");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
