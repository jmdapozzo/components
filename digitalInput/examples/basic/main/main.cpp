#include <stdio.h>
#include <esp_log.h>
#include <digitalInput.hpp>

#define CONFIG_GPIO_SWITCH_UP_OPTION 6
#define CONFIG_GPIO_SWITCH_DOWN 7

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Digital Input Test Program");

    macdap::DigitalInput &digitalInput = macdap::DigitalInput::getInstance();

    digitalInput.addInput(static_cast<gpio_num_t>(CONFIG_GPIO_SWITCH_UP_OPTION));
    digitalInput.addInput(static_cast<gpio_num_t>(CONFIG_GPIO_SWITCH_DOWN));

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
