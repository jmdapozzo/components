#pragma once

#include <esp_err.h>
#include "led_indicator_strips.h"

namespace macdap
{
    enum class Status
    {
        Booting,
        Provisioning,
        Connecting,
        Running,
        OtaUpdating,
        Error,
        Count
    };

    class StatusLed
    {

    private:
        led_indicator_handle_t m_led_indicator_handle;
        int8_t m_brightness = CONFIG_STATUS_LED_DEFAULT_BRIGHTNESS;
        StatusLed();
        ~StatusLed();

    public:
        StatusLed(StatusLed const&) = delete;
        void operator=(StatusLed const &) = delete;
        static StatusLed &get_instance()
        {
            static StatusLed instance;
            return instance;
        }

        void clear(Status status);
        void set_on_off(bool on_off);
        void set_brightness(int8_t brightness);
        void set_status(Status status);
    };
}
