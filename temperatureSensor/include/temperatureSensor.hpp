#pragma once

#include "esp_err.h"
#include <driver/temperature_sensor.h>
#include <driver/i2c_master.h>

#ifdef __cplusplus
extern "C"
{
#endif

namespace macdap
{
    typedef enum {
        celcius,
        farenheit
    } temperature_unit_t;

    typedef enum {
        comparator,         // In comparitor mode, the sensor acts like a thermostat and will drive the INT pin
        interrupt           // In interrupt mode the INT pin is activated when a temperature fault is detected and cleared
    } mode_t;

    typedef enum {
        faultCount1,        // Raise an alert after 1 fault
        faultCount2,        // Raise an alert after 2 faults
        faultCount4,        // Raise an alert after 4 faults
        faultCount6,        // Raise an alert after 6 faults
    } faultCount_t;

    class TemperatureSensor
    {

    private:
        temperature_sensor_handle_t m_temperatureSensorHandle;
        i2c_master_dev_handle_t m_i2cDeviceHandle;
        TemperatureSensor();
        ~TemperatureSensor();

    public:
        TemperatureSensor(TemperatureSensor const&) = delete;
        void operator=(TemperatureSensor const &) = delete;
        static TemperatureSensor &getInstance()
        {
            static TemperatureSensor instance;
            return instance;
        }

        esp_err_t getCPUTemperature(float *temperature, temperature_unit_t unit = celcius);
        esp_err_t getExternalTemperature(float *temperature, temperature_unit_t unit = celcius);

        esp_err_t getSensorIdleTime(float *idleTime);
        esp_err_t setSensorIdleTime(float idleTime);

        esp_err_t setSensorActiveHigh(bool activeHigh);

        esp_err_t getSensorHighTemperatureThreshold(float *temperatureThreshold);
        esp_err_t setSensorHighTemperatureThreshold(float temperatureThreshold);

        esp_err_t getSensorTemperatureHysteresis(float *temperatureHysteresis);
        esp_err_t setSensorTemperatureHysteresis(float temperatureHysteresis);

        esp_err_t getSensorMode(mode_t *mode);
        esp_err_t setSensorMode(mode_t mode);

        esp_err_t getSensorFaultCount(faultCount_t *faultCount);
        esp_err_t setSensorFaultCount(faultCount_t faultCount);
    };
}

#ifdef __cplusplus
}
#endif