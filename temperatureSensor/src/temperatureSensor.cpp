#include "temperatureSensor.hpp"
#include <esp_log.h>
#include <esp_err.h>
#include <esp_check.h>

//Code taken from https://github.com/adafruit/Adafruit_PCT2075/blob/master/Adafruit_PCT2075.cpp

using namespace macdap;

#define PCT2075_I2CADDR_DEFAULT     0x37    // Address is configured with pins A0-A2
#define PCT2075_REGISTER_TEMP       0x00    // Temperature register (read-only)
#define PCT2075_REGISTER_CONFIG     0x01    // Configuration register
#define PCT2075_REGISTER_THYST      0x02    // Hysterisis register
#define PCT2075_REGISTER_TOS        0x03    // OS register
#define PCT2075_REGISTER_TIDLE      0x04    // Measurement idle time registerconfiguration register

static const char *TAG = "temperature";

TemperatureSensor::TemperatureSensor()
{
    ESP_LOGI(TAG, "Initializing...");

    ESP_LOGI(TAG, "Setup the internal temperature sensor, set min/max values to -10 ~ 80 °C");
    temperature_sensor_config_t temperature_sensor_config = {
        .range_min = -10,
        .range_max = 80,
    };
    ESP_ERROR_CHECK(temperature_sensor_install(&temperature_sensor_config, &m_temperature_sensor_handle));

    i2c_master_bus_handle_t i2c_master_bus_handle;
    ESP_ERROR_CHECK(i2c_master_get_bus_handle(0, &i2c_master_bus_handle));
    if (i2c_master_probe(i2c_master_bus_handle, CONFIG_I2C_TEMPERATURE_SENSOR_ADDR, 100) != ESP_OK)
    {
        ESP_LOGW(TAG, "I2C device not found");
        return;
    }

    ESP_LOGI(TAG, "Found device at 0x37");
    i2c_device_config_t i2c_device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CONFIG_I2C_TEMPERATURE_SENSOR_ADDR,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_master_bus_handle, &i2c_device_config, &m_i2c_device_handle));
}

TemperatureSensor::~TemperatureSensor()
{
}

esp_err_t TemperatureSensor::get_cpu_temperature(float *temperature, temperature_unit_t unit)
{
    esp_err_t result = ESP_OK;
    float celcius_temperature;

    result = temperature_sensor_enable(m_temperature_sensor_handle);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to enable CPU temperature sensor");

    result = temperature_sensor_get_celsius(m_temperature_sensor_handle, &celcius_temperature);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to get CPU temperature");

    result = temperature_sensor_disable(m_temperature_sensor_handle);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to disable CPU temperature sensor");

    if (unit == Fahrenheit)
    {
        *temperature = celcius_temperature * 9 / 5 + 32;
    }
    else
    {
        *temperature = celcius_temperature;
    }

    return ESP_OK;
}

esp_err_t TemperatureSensor::get_external_temperature(float *temperature, temperature_unit_t unit)
{
    uint8_t buf[1] = {PCT2075_REGISTER_TEMP};
    uint8_t buffer[2];
    esp_err_t result = i2c_master_transmit_receive(m_i2c_device_handle, buf, sizeof(buf), buffer, 2, -1);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to read temperature sensor");

    uint16_t raw_temperature = (buffer[0] << 8) | buffer[1];

    float celcius_temperature = static_cast<float>((raw_temperature >> 5) * 0.125);
    if (unit == Fahrenheit)
    {
        *temperature = celcius_temperature * 9 / 5 + 32;
    }
    else
    {
        *temperature = celcius_temperature;
    }

    return ESP_OK;
}

esp_err_t TemperatureSensor::get_sensor_idle_time(float *idle_time)
{
    ESP_RETURN_ON_ERROR(ESP_ERR_NOT_SUPPORTED, TAG, "Failed to read temperature sensor idle time");
    return ESP_OK;
}

esp_err_t TemperatureSensor::set_sensor_idle_time(float idle_time)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t TemperatureSensor::set_sensor_active_high(bool active_high)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t TemperatureSensor::get_sensor_high_temperature_threshold(float *temperature_threshold)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t TemperatureSensor::set_sensor_high_temperature_threshold(float temperature_threshold)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t TemperatureSensor::get_sensor_temperature_hysteresis(float *temperature_hysteresis)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t TemperatureSensor::set_sensor_temperature_hysteresis(float temperature_hysteresis)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t get_sensor_mode(mode_t *mode)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t TemperatureSensor::set_sensor_mode(mode_t mode)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t get_sensor_fault_count(faultCount_t *fault_count)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t TemperatureSensor::set_sensor_fault_count(faultCount_t fault_count)
{
    return ESP_ERR_NOT_SUPPORTED;
}
