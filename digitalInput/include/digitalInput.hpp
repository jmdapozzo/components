#pragma once

#include "eventLoop.hpp"
#include "driver/gpio.h"
#include "driver/gpio_filter.h"

ESP_EVENT_DECLARE_BASE(GPIO_EVENTS);

namespace macdap
{

    typedef enum
    {
        Falling = 0,
        Rising = 1
    } EdgeDetection_t;

    class DigitalInput
    {

    private:
        DigitalInput();
        ~DigitalInput();

    public:
        DigitalInput(DigitalInput const&) = delete;
        void operator=(DigitalInput const &) = delete;
        static DigitalInput &get_instance()
        {
            static DigitalInput instance;
            return instance;
        }
        esp_err_t add_input(gpio_num_t gpioNumber);
        esp_err_t remove_input(gpio_num_t gpioNumber);
    };
}
