#pragma once

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#ifdef __cplusplus
extern "C"
{
#endif

    namespace macdap
    {
        class BleManager
        {

        private:
            TaskHandle_t m_taskHandle = nullptr;
            SemaphoreHandle_t m_semaphoreHandle;
            BleManager();
            ~BleManager();

        public:
            BleManager(BleManager const&) = delete;
            void operator=(BleManager const &) = delete;
            static BleManager &getInstance()
            {
                static BleManager instance;
                return instance;
            }

            esp_err_t start();
            esp_err_t stop();
        };
    }
#ifdef __cplusplus
}
#endif