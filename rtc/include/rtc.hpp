#pragma once

#include "esp_err.h"
#include "esp_event.h"
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <driver/i2c_master.h>
#include <time.h>
#include <sys/time.h>

ESP_EVENT_DECLARE_BASE(RTC_EVENTS);

namespace macdap
{
    typedef enum {
        RTCTimeUpdate,
        RTCAlarm1Triggered,
        RTCAlarm2Triggered
    } rtc_event_id_t;

    typedef struct {
        struct tm time_info;
        bool is_valid;
        bool is_running;
    } rtc_data_t;

    typedef enum {
        Alarm1,
        Alarm2
    } alarm_t;

    typedef enum {
        AlarmMatchSeconds,           // Alarm when seconds match
        AlarmMatchMinutes,           // Alarm when minutes and seconds match
        AlarmMatchHours,             // Alarm when hours, minutes, and seconds match
        AlarmMatchDay,               // Alarm when day, hours, minutes, and seconds match
        AlarmMatchDate,              // Alarm when date, hours, minutes, and seconds match
        AlarmMatchAll                // Alarm when all date/time values match
    } alarm_match_t;

    class RTC
    {
    private:
        bool m_is_present;
        i2c_master_dev_handle_t m_i2c_device_handle;
        SemaphoreHandle_t m_semaphore_handle;
        esp_event_loop_handle_t m_event_loop_handle;
        bool m_battery_enabled;
        RTC();
        ~RTC();

        // Internal helper methods
        esp_err_t read_register(uint8_t reg_addr, uint8_t *data, size_t len);
        esp_err_t write_register(uint8_t reg_addr, const uint8_t *data, size_t len);
        uint8_t bcd_to_dec(uint8_t bcd);
        uint8_t dec_to_bcd(uint8_t dec);

    public:
        RTC(RTC const&) = delete;
        void operator=(RTC const &) = delete;
        static RTC &get_instance()
        {
            static RTC instance;
            return instance;
        }

        bool is_present() const { return m_is_present; }
        esp_event_loop_handle_t get_event_loop_handle() const { return m_event_loop_handle; }
        void set_event_loop_handle(esp_event_loop_handle_t event_loop_handle) { m_event_loop_handle = event_loop_handle; }

        // Core RTC functions
        esp_err_t get_time(struct tm *time_info);
        esp_err_t set_time(const struct tm *time_info);
        esp_err_t is_running(bool *running);
        esp_err_t start();
        esp_err_t stop();

        // System time synchronization
        esp_err_t sync_from_system();      // Copy system time to RTC
        esp_err_t sync_to_system();        // Copy RTC time to system

        // Battery backup
        esp_err_t enable_battery_backup(bool enable);
        esp_err_t is_battery_enabled(bool *enabled);

        // Power fail/restore timestamps
        esp_err_t get_power_down_timestamp(struct tm *time_info);
        esp_err_t get_power_up_timestamp(struct tm *time_info);
        esp_err_t has_lost_power(bool *lost_power);
        esp_err_t clear_power_fail_flag();

        // Alarm functions
        esp_err_t set_alarm(alarm_t alarm, const struct tm *time_info, alarm_match_t match_mode);
        esp_err_t enable_alarm(alarm_t alarm, bool enable);
        esp_err_t is_alarm_triggered(alarm_t alarm, bool *triggered);
        esp_err_t clear_alarm_flag(alarm_t alarm);

        // Square wave output (MFP pin)
        esp_err_t enable_square_wave(bool enable);
        esp_err_t set_square_wave_frequency(uint8_t freq);  // 0=1Hz, 1=4.096kHz, 2=8.192kHz, 3=32.768kHz

        // SRAM access (64 bytes on MCP7940)
        esp_err_t read_sram(uint8_t address, uint8_t *data, size_t len);
        esp_err_t write_sram(uint8_t address, const uint8_t *data, size_t len);

        // Event handling
        esp_err_t register_event_handler(esp_event_handler_t event_handler, void* handler_arg = nullptr);
        esp_err_t unregister_event_handler(esp_event_handler_t event_handler);
    };
}
