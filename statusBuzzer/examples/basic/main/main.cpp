#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <statusBuzzer.hpp>

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Status Buzzer Test Program");

    macdap::StatusBuzzer &statusBuzzer = macdap::StatusBuzzer::get_instance();

    // Initialize the buzzer on GPIO 5 (or your preferred GPIO)
    esp_err_t ret = statusBuzzer.init(GPIO_NUM_5);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize buzzer");
        return;
    }

    while (true)
    {
        ESP_LOGI(TAG, "BuzzerStatus::SlowChirp");
        statusBuzzer.set_status(macdap::BuzzerStatus::SlowChirp);
        vTaskDelay(pdMS_TO_TICKS(10000));

        ESP_LOGI(TAG, "BuzzerStatus::MediumChirp");
        statusBuzzer.set_status(macdap::BuzzerStatus::MediumChirp);
        vTaskDelay(pdMS_TO_TICKS(10000));

        ESP_LOGI(TAG, "BuzzerStatus::FastChirp");
        statusBuzzer.set_status(macdap::BuzzerStatus::FastChirp);
        vTaskDelay(pdMS_TO_TICKS(10000));

        ESP_LOGI(TAG, "BuzzerStatus::SlowBeep");
        statusBuzzer.set_status(macdap::BuzzerStatus::SlowBeep);
        vTaskDelay(pdMS_TO_TICKS(10000));

        ESP_LOGI(TAG, "BuzzerStatus::MediumBeep");
        statusBuzzer.set_status(macdap::BuzzerStatus::MediumBeep);
        vTaskDelay(pdMS_TO_TICKS(10000));

        ESP_LOGI(TAG, "BuzzerStatus::FastBeep");
        statusBuzzer.set_status(macdap::BuzzerStatus::FastBeep);
        vTaskDelay(pdMS_TO_TICKS(10000));

        ESP_LOGI(TAG, "BuzzerStatus::Continuous");
        statusBuzzer.set_status(macdap::BuzzerStatus::Continuous);
        vTaskDelay(pdMS_TO_TICKS(5000));

        ESP_LOGI(TAG, "BuzzerStatus::Silent");
        statusBuzzer.stop();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
