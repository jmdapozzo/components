#pragma once

#include "esp_err.h"
#include <driver/temperature_sensor.h>
#include <driver/i2c_master.h>

namespace macdap
{
    typedef enum {
        Celsius,
        Fahrenheit
    } temperature_unit_t;

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
    };
}
