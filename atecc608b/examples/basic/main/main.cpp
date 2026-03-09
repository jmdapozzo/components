#include <stdio.h>
#include <esp_log.h>
#include <atecc608b.hpp>
#include "crypto_manager.h"

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Crypto Test Program");

    init_crypto_manager();

}
