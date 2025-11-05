#pragma once

#include "esp_err.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

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
        bool m_initialized = false;
        TaskHandle_t m_task_handle = nullptr;
        Management();
        ~Management();

    public:
        Management(Management const &) = delete;
        void operator=(Management const &) = delete;
        static Management &get_instance()
        {
            static Management instance;
            return instance;
        }
    };
}
