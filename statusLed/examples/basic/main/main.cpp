#include <stdio.h>
#include <esp_log.h>
#include <statusLed.hpp>

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Status Led Test Program");

    macdap::StatusLed &statusLed = macdap::StatusLed::get_instance();

    ESP_LOGI(TAG, "Initial Status Led State");

    while (true)
    {
        statusLed.set_status(macdap::Status::Off);
        ESP_LOGI(TAG, "Status::Off");
        vTaskDelay(pdMS_TO_TICKS(1500));

        statusLed.set_status(macdap::Status::Booting);
        ESP_LOGI(TAG, "Status::Booting");
        vTaskDelay(pdMS_TO_TICKS(5100));

        statusLed.set_status(macdap::Status::Commissioning);
        ESP_LOGI(TAG, "Status::Commissioning");
        vTaskDelay(pdMS_TO_TICKS(5010));

        statusLed.set_status(macdap::Status::WaitingForNetwork);
        ESP_LOGI(TAG, "Status::WaitingForNetwork");
        vTaskDelay(pdMS_TO_TICKS(1500));

        statusLed.set_status(macdap::Status::Running);
        ESP_LOGI(TAG, "Status::Running");
        vTaskDelay(pdMS_TO_TICKS(1500));

        statusLed.set_status(macdap::Status::Error);
        ESP_LOGI(TAG, "Status::Error");
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}
