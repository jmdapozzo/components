#pragma once

#include <esp_err.h>
#include "esp_event.h"
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <vector>
#include <driver/i2c_master.h>
#include <lvgl.h>
#include <esp_lvgl_port.h>

ESP_EVENT_DECLARE_BASE(LIGHT_SENSOR_EVENTS);

namespace macdap
{
    typedef enum {
        LightSensorUpdate
    } light_sensor_event_id_t;

    typedef enum IntensityStatus
    {
        Lowest,
        Low,
        Medium,
        High,
        Highest
    } intensity_status_t;

    typedef struct {
        uint16_t illuminance;                  // Measured illuminance in lux
        intensity_status_t intensity_status;   // Intensity status
    } light_sensor_data_t;

    typedef enum {
        ContinuouslyHResolutionMode     = 0x10,   // Command to set measure mode as Continuously H-Resolution mode
        ContinuouslyHResolutionMode2    = 0x11,   // Command to set measure mode as Continuously H-Resolution mode2
        ContinuouslyLResolutionMode     = 0x13,   // Command to set measure mode as Continuously L-Resolution mode
        OneTimeHResolutionMode          = 0x20,   // Command to set measure mode as One Time H-Resolution mode
        OneTimeHResolutionMode2         = 0x21,   // Command to set measure mode as One Time H-Resolution mode2
        OneTimeLResolutionMode          = 0x23,   // Command to set measure mode as One Time L-Resolution mode
    } measure_mode_t;

    class LightSensor
    {

    private:
        i2c_master_dev_handle_t m_i2c_device_handle;
        SemaphoreHandle_t m_semaphore_handle;
        esp_timer_handle_t m_periodic_timer;
        esp_event_loop_handle_t m_event_loop_handle;
        std::vector<lv_obj_t*> m_icons;
        LightSensor();
        ~LightSensor();

    public:
        LightSensor(LightSensor const&) = delete;
        void operator=(LightSensor const &) = delete;
        static LightSensor &get_instance()
        {
            static LightSensor instance;
            return instance;
        }
        esp_event_loop_handle_t get_event_loop_handle() { return m_event_loop_handle; }
        void set_event_loop_handle(esp_event_loop_handle_t event_loop_handle) { m_event_loop_handle = event_loop_handle; }        
        std::vector<lv_obj_t*> get_icons_vector() { return m_icons; }
        esp_err_t reset();
        esp_err_t power_down();
        esp_err_t power_on();
        esp_err_t set_measure_mode(const measure_mode_t measure_mode);
        esp_err_t get_illuminance(uint16_t *illuminance, intensity_status_t *intensity_status = nullptr);
        esp_err_t set_measure_time(const uint8_t measure_time);
        esp_err_t add_lv_obj_icon(lv_obj_t *lv_obj_icon);
        esp_err_t register_event_handler(esp_event_handler_t event_handler, void* handler_arg = nullptr);
        esp_err_t unregister_event_handler(esp_event_handler_t event_handler);
    };
}
