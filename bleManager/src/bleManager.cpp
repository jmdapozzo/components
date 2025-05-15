#include "bleManager.hpp"
#include "string.h"
#include "esp_log.h"

#define LOCAL_TASK_STACKSIZE 4000 

#define DEVICE_INFO_SERVICE 0x180A
#define MANUFACTURER_NAME 0x2A29

using namespace macdap;

static const char *TAG = "bleManager";

static void localTask(void *parameter)
{
    //BleManager *bleManager = (BleManager *)parameter;

    char *taskName = pcTaskGetName(nullptr);
    ESP_LOGI(TAG, "Starting %s", taskName);

    while (true)
    {
        TickType_t nextRefresh = 10;

        ESP_LOGI(TAG, "Next BLE refresh in %lu seconds", nextRefresh);
        nextRefresh = nextRefresh * (1000 / portTICK_PERIOD_MS);
        vTaskDelay(nextRefresh);
    }

    vTaskDelete(NULL);
}

BleManager::BleManager()
{
    ESP_LOGI(TAG, "Initializing...");

    m_semaphoreHandle = xSemaphoreCreateMutex();

    if (m_semaphoreHandle != nullptr)
    {
        if (m_taskHandle == nullptr)
        {
            if (xTaskCreate(
                    localTask,
                    TAG,
                    LOCAL_TASK_STACKSIZE,
                    this,
                    tskIDLE_PRIORITY,
                    &m_taskHandle) != pdPASS)
            {
                ESP_LOGE(TAG, "xTaskCreate failed");
            }
        }
        else
        {
            ESP_LOGW(TAG, "BleManager already initialized");
        }
    }
    else
    {
        ESP_LOGE(TAG, "xSemaphoreCreateMutex failed");
    }
}

BleManager::~BleManager()
{
    if (m_taskHandle != nullptr)
    {
        vTaskDelete(m_taskHandle);
    }

    if (m_semaphoreHandle != nullptr)
    {
        vSemaphoreDelete(m_semaphoreHandle);
    }
}

esp_err_t BleManager::start()
{
    esp_err_t result = ESP_OK;

    return result;
}

esp_err_t BleManager::stop()
{
    esp_err_t result = ESP_OK;

    return result;
}

