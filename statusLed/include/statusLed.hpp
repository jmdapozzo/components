#pragma once

#include "esp_err.h"
#include "led_strip.h"

#ifdef __cplusplus
extern "C"
{
#endif

namespace macdap
{
    enum class Status
    {
        Off,
        Booting,
        Commissionning,
        RunningPhase0,
        RunningPhase1,
        Error
    };

    class StatusLed
    {

    private:
#ifdef CONFIG_LED_TYPE_COLOR
        led_strip_handle_t m_ledStrip;
#endif
        Status status = Status::Off;
        int8_t brightness = CONFIG_STATUS_LED_DEFAULT_BRIGHTNESS;
        StatusLed();
        ~StatusLed();

    public:
        StatusLed(StatusLed const&) = delete;
        void operator=(StatusLed const &) = delete;
        static StatusLed &getInstance()
        {
            static StatusLed instance;
            return instance;
        }

        void clear();
        void setStatus(Status status);
    };
}

#ifdef __cplusplus
}
#endif