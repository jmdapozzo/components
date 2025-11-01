#pragma once

#include <esp_err.h>
#include <led_strip.h>

namespace macdap
{
    enum class Status
    {
        Off,
        Booting,
        Commissioning,
        WaitingForNetwork,
        Running,
        Error
    };

    class StatusLed
    {

    private:
#ifdef CONFIG_STATUS_LED_TYPE_COLOR
        led_strip_handle_t m_ledStrip;
#endif
        Status status = Status::Off;
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

        void clear();
        void set_status(Status status);
    };
}
