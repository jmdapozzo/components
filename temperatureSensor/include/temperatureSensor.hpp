#pragma once

#include "esp_err.h"
#include "esp_event.h"
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <vector>
#include <driver/temperature_sensor.h>
#include <driver/i2c_master.h>
#include <lvgl.h>
#include <esp_lvgl_port.h>

ESP_EVENT_DECLARE_BASE(TEMPERATURE_SENSOR_EVENTS);

namespace macdap
{
    typedef enum {
        TemperatureSensorUpdate
    } temperature_sensor_event_id_t;

    typedef enum {
        Celsius,
        Fahrenheit
    } temperature_unit_t;

    typedef enum {
        both,
        external,
        cpu
    } temperature_sensor_type_t;

    typedef struct {
        float external_temperature;
        float cpu_temperature;
        temperature_unit_t temperature_unit;
    } temperature_sensor_data_t;

    typedef enum {
        Comparator,         // In comparator mode, the sensor acts like a thermostat and will drive the INT pin
        Interrupt           // In interrupt mode the INT pin is activated when a temperature fault is detected and cleared
    } mode_t;

    typedef enum {
        FaultCount1,        // Raise an alert after 1 fault
        FaultCount2,        // Raise an alert after 2 faults
        FaultCount4,        // Raise an alert after 4 faults
        FaultCount6,        // Raise an alert after 6 faults
    } faultCount_t;

    class TemperatureSensor
    {

    private:
        temperature_sensor_handle_t m_temperature_sensor_handle;
        i2c_master_dev_handle_t m_i2c_device_handle;
        SemaphoreHandle_t m_semaphore_handle;
        esp_timer_handle_t m_periodic_timer;
        esp_event_loop_handle_t m_event_loop_handle;
        temperature_unit_t m_default_unit;
        std::vector<lv_obj_t*> m_labels;
        TemperatureSensor();
        ~TemperatureSensor();

    public:
        TemperatureSensor(TemperatureSensor const&) = delete;
        void operator=(TemperatureSensor const &) = delete;
        static TemperatureSensor &get_instance()
        {
            static TemperatureSensor instance;
            return instance;
        }
        esp_event_loop_handle_t get_event_loop_handle() const { return m_event_loop_handle; }
        void set_event_loop_handle(esp_event_loop_handle_t event_loop_handle) { m_event_loop_handle = event_loop_handle; }
        std::vector<lv_obj_t*> get_labels_vector() const { return m_labels; }
        temperature_unit_t get_default_unit() const { return m_default_unit; }

        esp_err_t get_cpu_temperature(float *temperature, temperature_unit_t unit = Celsius);
        esp_err_t get_external_temperature(float *temperature, temperature_unit_t unit = Celsius);

        esp_err_t get_sensor_idle_time(float *idle_time);
        esp_err_t set_sensor_idle_time(float idle_time);

        esp_err_t set_sensor_active_high(bool active_high);

        esp_err_t get_sensor_high_temperature_threshold(float *temperature_threshold);
        esp_err_t set_sensor_high_temperature_threshold(float temperature_threshold);

        esp_err_t get_sensor_temperature_hysteresis(float *temperature_hysteresis);
        esp_err_t set_sensor_temperature_hysteresis(float temperature_hysteresis);

        esp_err_t get_sensor_mode(mode_t *mode);
        esp_err_t set_sensor_mode(mode_t mode);

        esp_err_t get_sensor_fault_count(faultCount_t *fault_count);
        esp_err_t set_sensor_fault_count(faultCount_t fault_count);

        esp_err_t add_lv_obj_label(lv_obj_t *lv_obj_label, temperature_sensor_type_t temperature_sensor_type = external);
        esp_err_t register_event_handler(esp_event_handler_t event_handler, temperature_unit_t default_unit = Celsius, void* handler_arg = nullptr);
        esp_err_t unregister_event_handler(esp_event_handler_t event_handler);
    };
}
