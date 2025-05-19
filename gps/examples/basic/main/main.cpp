#include <stdio.h>
#include <esp_log.h>
#include <gps.hpp>

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "GPS Test Program");

    macdap::GPS &gps = macdap::GPS::getInstance();
}
