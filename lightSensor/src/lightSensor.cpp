#include "lightSensor.hpp"
#include <esp_log.h>
#include <esp_err.h>
#include <esp_check.h>

using namespace macdap;

#define BH_1750_MEASUREMENT_ACCURACY    1.2

#define BH1750_POWER_DOWN   0x00
#define BH1750_POWER_ON     0x01
#define BH1750_POWER_RESET  0x07

static const char *TAG = "lightSensor";

LightSensor::LightSensor()
{
    ESP_LOGI(TAG, "Initializing...");

    i2c_master_bus_handle_t i2cMasterBusHandle;
    ESP_ERROR_CHECK(i2c_master_get_bus_handle(0, &i2cMasterBusHandle));
    if (i2c_master_probe(i2cMasterBusHandle, CONFIG_I2C_LIGHT_SENSOR_ADDR, 100) != ESP_OK)
    {
        ESP_LOGW(TAG, "I2C device not found");
        return;
    }

    ESP_LOGI(TAG, "Found device at 0x23");
    i2c_device_config_t i2cDeviceConfig = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CONFIG_I2C_LIGHT_SENSOR_ADDR,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2cMasterBusHandle, &i2cDeviceConfig, &m_i2cDeviceHandle));

    ESP_ERROR_CHECK(powerOn());
    ESP_ERROR_CHECK(setMeasureMode(measure_mode_t::ContinuouslyHResolutionMode));
}

LightSensor::~LightSensor()
{
}

esp_err_t LightSensor::reset()
{
    esp_err_t result = ESP_OK;

    uint8_t data = BH1750_POWER_RESET;
    result = i2c_master_transmit(m_i2cDeviceHandle, &data, 1, -1);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to power down light sensor");

    return ESP_OK;
}

esp_err_t LightSensor::powerDown()
{
    esp_err_t result = ESP_OK;

    uint8_t data = BH1750_POWER_DOWN;
    result = i2c_master_transmit(m_i2cDeviceHandle, &data, 1, -1);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to power down light sensor");

    return ESP_OK;
}

esp_err_t LightSensor::powerOn()
{
    esp_err_t result = ESP_OK;

    uint8_t data = BH1750_POWER_ON;
    result = i2c_master_transmit(m_i2cDeviceHandle, &data, 1, -1);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to power on light sensor");

    return ESP_OK;
}

esp_err_t LightSensor::setMeasureMode(const measure_mode_t measureMode)
{
    esp_err_t result = ESP_OK;

    uint8_t mode = static_cast<uint8_t>(measureMode);
    result = i2c_master_transmit(m_i2cDeviceHandle, &mode, 1, -1);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to set measure mode");

    return ESP_OK;
}

esp_err_t LightSensor::getIlluminance(float *illuminance)
{
    uint8_t buffer[2];
    esp_err_t result = i2c_master_receive(m_i2cDeviceHandle, buffer, 2, -1);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to read illuminance");

    uint16_t rawIlluminance = (buffer[0] << 8) | buffer[1];

    *illuminance = static_cast<float>(rawIlluminance);

    return ESP_OK;
}

esp_err_t LightSensor::setMeasureTime(const uint8_t measureTime)
{
    esp_err_t result = ESP_OK;
    uint8_t data[2] = {0x40, 0x60}; // constant part of the the MTreg
    data[0] |= measureTime >> 5;
    data[1] |= measureTime & 0x1F;
    result = i2c_master_transmit(m_i2cDeviceHandle, data, sizeof(data), -1);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to set measure time");

    return ESP_OK;
}



