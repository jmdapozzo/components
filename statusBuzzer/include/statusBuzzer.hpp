#pragma once

#include <esp_err.h>
#include <driver/ledc.h>
#include <esp_timer.h>

namespace macdap
{
    enum class BuzzerStatus
    {
        Silent,
        SlowChirp,
        MediumChirp,
        FastChirp,
        SlowBeep,
        MediumBeep,
        FastBeep,
        Continuous,
        Count
    };

    class StatusBuzzer
    {
    private:
        static constexpr uint32_t BUZZER_FREQUENCY = 4000; // Hz - SMT-0540 optimal frequency
        static constexpr ledc_timer_bit_t LEDC_DUTY_RESOLUTION = LEDC_TIMER_10_BIT;
        static constexpr uint32_t SQUARE_WAVE_DUTY_CYCLE = 50; // 50% duty cycle for square wave

        ledc_channel_t m_ledc_channel;
        ledc_timer_t m_ledc_timer;
        gpio_num_t m_gpio_num;
        esp_timer_handle_t m_pattern_timer;
        BuzzerStatus m_current_status;
        bool m_tone_active;

        StatusBuzzer();
        ~StatusBuzzer();

        static void pattern_timer_callback(void *arg);
        void handle_pattern_timer();
        void tone_on();
        void tone_off();
        void start_pattern(BuzzerStatus status);

    public:
        StatusBuzzer(StatusBuzzer const&) = delete;
        void operator=(StatusBuzzer const &) = delete;
        static StatusBuzzer &get_instance()
        {
            static StatusBuzzer instance;
            return instance;
        }

        esp_err_t init(gpio_num_t gpio_num, ledc_timer_t timer = LEDC_TIMER_0, ledc_channel_t channel = LEDC_CHANNEL_0);
        void set_status(BuzzerStatus status);
        void stop();
    };
}
