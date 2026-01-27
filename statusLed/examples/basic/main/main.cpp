#include <stdio.h>
#include <esp_log.h>
#include <statusLed.hpp>

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Status Led Test Program");

    macdap::StatusLed &statusLed = macdap::StatusLed::get_instance();

    while (true)
    {
        statusLed.set_status(macdap::Status::Booting);
        ESP_LOGI(TAG, "Status::Booting");
        vTaskDelay(pdMS_TO_TICKS(5000));

        statusLed.set_status(macdap::Status::Provisioning);
        ESP_LOGI(TAG, "Status::Provisioning");
        vTaskDelay(pdMS_TO_TICKS(5000));

        statusLed.set_status(macdap::Status::Connecting);
        ESP_LOGI(TAG, "Status::Connecting");
        vTaskDelay(pdMS_TO_TICKS(5000));

        statusLed.set_status(macdap::Status::Running);
        ESP_LOGI(TAG, "Status::Running");
        vTaskDelay(pdMS_TO_TICKS(5000));

        statusLed.set_status(macdap::Status::OtaUpdating);
        ESP_LOGI(TAG, "Status::OtaUpdating");
        vTaskDelay(pdMS_TO_TICKS(5000));

        statusLed.set_status(macdap::Status::Erasing);
        ESP_LOGI(TAG, "Status::Erasing");
        vTaskDelay(pdMS_TO_TICKS(5000));

        statusLed.set_status(macdap::Status::Error);
        ESP_LOGI(TAG, "Status::Error");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
