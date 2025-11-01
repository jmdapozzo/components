#include <stdio.h>
#include <esp_log.h>
#include <digitalInput.hpp>

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Digital Input Test Program");

    macdap::DigitalInput &digitalInput = macdap::DigitalInput::get_instance();

    digitalInput.add_input(static_cast<gpio_num_t>(0));
    digitalInput.add_input(static_cast<gpio_num_t>(5));
    digitalInput.add_input(static_cast<gpio_num_t>(6));
    digitalInput.add_input(static_cast<gpio_num_t>(7));

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}