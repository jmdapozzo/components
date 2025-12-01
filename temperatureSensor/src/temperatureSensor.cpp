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

ESP_EVENT_DEFINE_BASE(TEMPERATURE_SENSOR_EVENTS);
    
static void timer_callback(void* arg)
{
    TemperatureSensor* instance = static_cast<TemperatureSensor*>(arg);

    temperature_unit_t default_unit = instance->get_default_unit();

    float external_temperature;
    if (instance->get_external_temperature(&external_temperature, default_unit) == ESP_OK) {
    } else {
        ESP_LOGE(TAG, "Failed to read external temperature");
    }

    float cpu_temperature;
    if (instance->get_cpu_temperature(&cpu_temperature, default_unit) == ESP_OK) {
    } else {
        ESP_LOGE(TAG, "Failed to read CPU temperature");
    }

    const char* unit_str = (default_unit == Celsius) ? "°C" : "°F";

    for (const auto& label : instance->get_labels_vector()) {
        if (lvgl_port_lock(0)) {
            temperature_sensor_type_t user_data = static_cast<temperature_sensor_type_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(label)));
            char formatted_text[64];
            switch (user_data) {
                case both:
                    snprintf(formatted_text, sizeof(formatted_text), "CPU: %0.1f%s, Ext: %0.1f%s", cpu_temperature, unit_str, external_temperature, unit_str);
                    break;
                case external:
                    snprintf(formatted_text, sizeof(formatted_text), "%0.1f%s", external_temperature, unit_str);
                    break;
                case cpu:
                    snprintf(formatted_text, sizeof(formatted_text), "%0.1f%s", cpu_temperature, unit_str);
                    break;
                default:
                    snprintf(formatted_text, sizeof(formatted_text), "CPU: %0.1f%s, Ext: %0.1f%s", cpu_temperature, unit_str, external_temperature, unit_str);
                    break;
            }
            lv_label_set_text(label, formatted_text);
            lvgl_port_unlock();
        }
    }

    esp_event_loop_handle_t event_loop_handle = instance->get_event_loop_handle();
    if (event_loop_handle) {
        temperature_sensor_data_t sensor_data {
            .external_temperature = external_temperature,
            .cpu_temperature = cpu_temperature,
            .temperature_unit = default_unit
        };

        esp_event_post_to(event_loop_handle, TEMPERATURE_SENSOR_EVENTS, TemperatureSensorUpdate, &sensor_data, sizeof(temperature_sensor_data_t), 100 / portTICK_PERIOD_MS);
    }
}

TemperatureSensor::TemperatureSensor()
{
    ESP_LOGI(TAG, "Initializing...");

    i2c_master_bus_handle_t i2c_master_bus_handle;
    ESP_ERROR_CHECK(i2c_master_get_bus_handle(0, &i2c_master_bus_handle));
    if (i2c_master_probe(i2c_master_bus_handle, CONFIG_I2C_TEMPERATURE_SENSOR_ADDR, 100) != ESP_OK)
    {
        ESP_LOGW(TAG, "I2C device not found");
        m_is_present = false;
        return;
    }
    m_is_present = true;

    ESP_LOGI(TAG, "Setup the internal temperature sensor, set min/max values to -10 ~ 80 °C");
    temperature_sensor_config_t temperature_sensor_config = {
        .range_min = -10,
        .range_max = 80,
    };
    ESP_ERROR_CHECK(temperature_sensor_install(&temperature_sensor_config, &m_temperature_sensor_handle));

    m_semaphore_handle = xSemaphoreCreateMutex();

    if (m_semaphore_handle != nullptr)
    {
        xSemaphoreTake(m_semaphore_handle, 0);

        ESP_LOGI(TAG, "Found device at 0x37");
        i2c_device_config_t i2c_device_config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = CONFIG_I2C_TEMPERATURE_SENSOR_ADDR,
            .scl_speed_hz = 100000,
        };

        ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_master_bus_handle, &i2c_device_config, &m_i2c_device_handle));

        xSemaphoreGive(m_semaphore_handle);

        if (CONFIG_TEMPERATURE_SENSOR_POLLING_INTERVAL_SEC != 0) {
            ESP_LOGI(TAG, "Temperature sensor polling interval set to %d Second(s).", CONFIG_TEMPERATURE_SENSOR_POLLING_INTERVAL_SEC);
            esp_timer_create_args_t timer_args = {
                .callback = timer_callback,
                .arg = this,
                .dispatch_method = ESP_TIMER_TASK,
                .name = "TemperatureSensorTimer"
            };

            esp_timer_create(&timer_args, &m_periodic_timer);
            esp_timer_start_periodic(m_periodic_timer, CONFIG_TEMPERATURE_SENSOR_POLLING_INTERVAL_SEC * 1000000);
        } else {
            ESP_LOGI(TAG, "Temperature sensor polling disabled");
        }

    }
    else
    {
        ESP_LOGE(TAG, "xSemaphoreCreateMutex failed");
    }

}

TemperatureSensor::~TemperatureSensor()
{
}

esp_err_t TemperatureSensor::get_cpu_temperature(float *temperature, temperature_unit_t unit)
{
    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    esp_err_t result = ESP_OK;
    float celcius_temperature;

    result = temperature_sensor_enable(m_temperature_sensor_handle);
    if (result != ESP_OK)
    {
        xSemaphoreGive(m_semaphore_handle);
        ESP_RETURN_ON_ERROR(result, TAG, "Failed to enable CPU temperature sensor");
    }

    result = temperature_sensor_get_celsius(m_temperature_sensor_handle, &celcius_temperature);
    if (result != ESP_OK)
    {
        xSemaphoreGive(m_semaphore_handle);
        ESP_RETURN_ON_ERROR(result, TAG, "Failed to read CPU temperature sensor");
    }

    result = temperature_sensor_disable(m_temperature_sensor_handle);
    if (result != ESP_OK)
    {
        xSemaphoreGive(m_semaphore_handle);
        ESP_RETURN_ON_ERROR(result, TAG, "Failed to disable CPU temperature sensor");
    }

    if (unit == Fahrenheit)
    {
        *temperature = celcius_temperature * 9 / 5 + 32;
    }
    else
    {
        *temperature = celcius_temperature;
    }

    xSemaphoreGive(m_semaphore_handle);

    return ESP_OK;
}

esp_err_t TemperatureSensor::get_external_temperature(float *temperature, temperature_unit_t unit)
{
    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    uint8_t buf[1] = {PCT2075_REGISTER_TEMP};
    uint8_t buffer[2] = {0};
    esp_err_t result = i2c_master_transmit_receive(m_i2c_device_handle, buf, sizeof(buf), buffer, 2, -1);
    if (result != ESP_OK)
    {
        xSemaphoreGive(m_semaphore_handle);
        ESP_RETURN_ON_ERROR(result, TAG, "Failed to read temperature sensor");
    }

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

    xSemaphoreGive(m_semaphore_handle);

    return ESP_OK;
}

esp_err_t TemperatureSensor::get_sensor_idle_time(float *idle_time)
{
    return ESP_ERR_NOT_SUPPORTED;
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

esp_err_t TemperatureSensor::add_lv_obj_label(lv_obj_t *lv_obj_label, temperature_sensor_type_t temperature_sensor_type)
{
    if (lv_obj_label == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    lv_obj_set_user_data(lv_obj_label, reinterpret_cast<void*>(static_cast<uintptr_t>(temperature_sensor_type)));
    m_labels.push_back(lv_obj_label);
    return ESP_OK;
}

esp_err_t TemperatureSensor::register_event_handler(esp_event_handler_t event_handler, temperature_unit_t default_unit, void* handler_arg)
{
    if (!event_handler) {
        return ESP_ERR_INVALID_ARG;
    }
    m_default_unit = default_unit;
    return esp_event_handler_register_with(m_event_loop_handle, TEMPERATURE_SENSOR_EVENTS, ESP_EVENT_ANY_ID, event_handler, handler_arg);
}

esp_err_t TemperatureSensor::unregister_event_handler(esp_event_handler_t event_handler)
{
    if (!event_handler) {
        return ESP_ERR_INVALID_ARG;
    }
    
    return esp_event_handler_unregister_with(m_event_loop_handle, TEMPERATURE_SENSOR_EVENTS, ESP_EVENT_ANY_ID, event_handler);
}
