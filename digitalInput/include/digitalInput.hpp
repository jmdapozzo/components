#pragma once

#include "eventLoop.hpp"
#include "driver/gpio.h"
#include "driver/gpio_filter.h"

ESP_EVENT_DECLARE_BASE(GPIO_EVENTS);

#ifdef __cplusplus
extern "C"
{
#endif

    namespace macdap
    {

        typedef enum
        {
            falling = 0,
            rising = 1
        } EdgeDetection_t;

        class DigitalInput
        {

        private:
            DigitalInput();
            ~DigitalInput();

        public:
            DigitalInput(DigitalInput const&) = delete;
            void operator=(DigitalInput const &) = delete;
            static DigitalInput &getInstance()
            {
                static DigitalInput instance;
                return instance;
            }
            esp_err_t addInput(gpio_num_t gpioNumber);
            esp_err_t removeInput(gpio_num_t gpioNumber);
        };
    }
#ifdef __cplusplus
}
#endif