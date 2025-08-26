#include <stdio.h>
#include <esp_log.h>
#include <statusLed.hpp>

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Status Led Test Program");

    macdap::StatusLed &statusLed = macdap::StatusLed::getInstance();

    ESP_LOGI(TAG, "Initial Status Led State");

    while (true)
    {
        statusLed.setStatus(macdap::Status::Off);
        ESP_LOGI(TAG, "Status::Off");
        vTaskDelay(pdMS_TO_TICKS(3000));

        statusLed.setStatus(macdap::Status::Booting);
        ESP_LOGI(TAG, "Status::Booting");
        vTaskDelay(pdMS_TO_TICKS(3000));

        statusLed.setStatus(macdap::Status::Commissioning);
        ESP_LOGI(TAG, "Status::Commissioning");
        vTaskDelay(pdMS_TO_TICKS(3000));

        statusLed.setStatus(macdap::Status::RunningPhase0);
        ESP_LOGI(TAG, "Status::RunningPhase0");
        vTaskDelay(pdMS_TO_TICKS(3000));

        statusLed.setStatus(macdap::Status::RunningPhase1);
        ESP_LOGI(TAG, "Status::RunningPhase1");
        vTaskDelay(pdMS_TO_TICKS(3000));

        statusLed.setStatus(macdap::Status::Error);
        ESP_LOGI(TAG, "Status::Error");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
