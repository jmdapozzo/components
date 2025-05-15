#pragma once

#include "esp_err.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#ifdef __cplusplus
extern "C"
{
#endif

    namespace macdap
    {
        typedef struct xUPDATE_FIRMWARE
        {
            char name[64];
            char type[8];
            char date[64];
            char time[64];
            uint32_t size;
            char version[32];
            char url[256];
        } updateFirmware_t;

        class Management
        {

        private:
            TaskHandle_t m_taskHandle = nullptr;
            Management();
            ~Management();

        public:
            Management(Management const &) = delete;
            void operator=(Management const &) = delete;
            char m_outputBuffer[4096];
            int64_t m_outputLength;
            static Management &getInstance()
            {
                static Management instance;
                return instance;
            }
        };
    }
#ifdef __cplusplus
}
#endif
