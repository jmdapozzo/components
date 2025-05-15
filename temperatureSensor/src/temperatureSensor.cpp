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
    temperature_sensor_config_t temperatureSensoConfig = {
        .range_min = -10,
        .range_max = 80,
    };
    ESP_ERROR_CHECK(temperature_sensor_install(&temperatureSensoConfig, &m_temperatureSensorHandle));

    i2c_master_bus_handle_t i2cMasterBusHandle;
    ESP_ERROR_CHECK(i2c_master_get_bus_handle(0, &i2cMasterBusHandle));
    if (i2c_master_probe(i2cMasterBusHandle, CONFIG_I2C_TEMPERATURE_SENSOR_ADDR, 100) != ESP_OK)
    {
        ESP_LOGW(TAG, "I2C device not found");
        return;
    }

    ESP_LOGI(TAG, "Found device at 0x37");
    i2c_device_config_t i2cDeviceConfig = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CONFIG_I2C_TEMPERATURE_SENSOR_ADDR,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2cMasterBusHandle, &i2cDeviceConfig, &m_i2cDeviceHandle));
}

TemperatureSensor::~TemperatureSensor()
{
}

esp_err_t TemperatureSensor::getCPUTemperature(float *temperature, temperature_unit_t unit)
{
    esp_err_t result = ESP_OK;
    float celciusTemperature;

    result = temperature_sensor_enable(m_temperatureSensorHandle);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to enable CPU temperature sensor");

    result = temperature_sensor_get_celsius(m_temperatureSensorHandle, &celciusTemperature);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to get CPU temperature");

    result = temperature_sensor_disable(m_temperatureSensorHandle);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to disable CPU temperature sensor");

    if (unit == farenheit)
    {
        *temperature = celciusTemperature * 9 / 5 + 32;
    }
    else
    {
        *temperature = celciusTemperature;
    }

    return ESP_OK;
}

esp_err_t TemperatureSensor::getExternalTemperature(float *temperature, temperature_unit_t unit)
{
    uint8_t buf[1] = {PCT2075_REGISTER_TEMP};
    uint8_t buffer[2];
    esp_err_t result = i2c_master_transmit_receive(m_i2cDeviceHandle, buf, sizeof(buf), buffer, 2, -1);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to read temperature sensor");

    uint16_t rawTemperature = (buffer[0] << 8) | buffer[1];

    float celciusTemperature = static_cast<float>((rawTemperature >> 5) * 0.125);
    if (unit == farenheit)
    {
        *temperature = celciusTemperature  * 9 / 5 + 32;
    }
    else
    {
        *temperature = celciusTemperature;
    }

    return ESP_OK;
}

esp_err_t TemperatureSensor::getSensorIdleTime(float *idleTime)
{
    ESP_RETURN_ON_ERROR(ESP_ERR_NOT_SUPPORTED, TAG, "Failed to read temperature sensor idle time");
    return ESP_OK;
}

esp_err_t TemperatureSensor::setSensorIdleTime(float idleTime)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t TemperatureSensor::setSensorActiveHigh(bool activeHigh)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t TemperatureSensor::getSensorHighTemperatureThreshold(float *temperatureThreshold)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t TemperatureSensor::setSensorHighTemperatureThreshold(float temperatureThreshold)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t TemperatureSensor::getSensorTemperatureHysteresis(float *temperatureHysteresis)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t TemperatureSensor::setSensorTemperatureHysteresis(float temperatureHysteresis)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t getSensorMode(mode_t *mode)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t TemperatureSensor::setSensorMode(mode_t mode)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t getSensorFaultCount(faultCount_t *faultCount)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t TemperatureSensor::setSensorFaultCount(faultCount_t)
{
    return ESP_ERR_NOT_SUPPORTED;
}
