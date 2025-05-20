#include <stdio.h>
#include <esp_log.h>
#include <ledPanel.hpp>

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "LED Panel Test Program");

    const lvgl_port_cfg_t lvglConfig = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvglConfig);

    // macdap::LedPanel ledPanel(macdap::w16h8, CONFIG_LED_PANEL_WIDTH, CONFIG_LED_PANEL_HEIGHT);
    macdap::LedPanel ledPanel(macdap::w16h8, 3, 2);
    ledPanel.setBrightness(1.00);
    ledPanel.scrollingMessage("Hello World from MacDap");

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
