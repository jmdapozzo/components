#include "lightSensor.hpp"
#include <esp_log.h>
#include <esp_err.h>
#include <esp_check.h>
#include <icons.h>

using namespace macdap;

#define BH_1750_MEASUREMENT_ACCURACY    1.2

#define BH1750_POWER_DOWN   0x00
#define BH1750_POWER_ON     0x01
#define BH1750_POWER_RESET  0x07

static const char *TAG = "lightSensor";

ESP_EVENT_DEFINE_BASE(LIGHT_SENSOR_EVENTS);

static void update_status(lv_obj_t *icon, intensity_status_t intensity_status)
{
    lv_coord_t height = lv_obj_get_height(icon);

    const lv_image_dsc_t *icon_src = nullptr;
    switch(height)
    {
        case 32:
            switch (intensity_status)
            {
                case Lowest:  icon_src = &sun_dim_thin_32; break;
                case Low:     icon_src = &sun_dim_32; break;
                case Medium:  icon_src = &sun_32; break;
                case High:    icon_src = &sun_bold_32; break;
                case Highest: icon_src = &sun_fill_32; break;
                default:      icon_src = &sun_32; break;
            }
        break;
        case 16:
            switch (intensity_status)
            {
                case Lowest:  icon_src = &sun_dim_thin_16; break;
                case Low:     icon_src = &sun_dim_16; break;
                case Medium:  icon_src = &sun_16; break;
                case High:    icon_src = &sun_bold_16; break;
                case Highest: icon_src = &sun_fill_16; break;
                default:      icon_src = &sun_16; break;
            }
        break;
        default:
            ESP_LOGE(TAG, "Unexpected Intensity icon height: %d", height);
        break;
    }

    if (icon_src != nullptr) {
        if (lvgl_port_lock(0)) {
            lv_img_set_src(icon, icon_src);
            lvgl_port_unlock();
        }
    }
}

static void timer_callback(void* arg)
{
    LightSensor* instance = static_cast<LightSensor*>(arg);

    uint16_t illuminance;
    intensity_status_t intensity_status = IntensityStatus::Lowest;
    if (instance->get_illuminance(&illuminance, &intensity_status) == ESP_OK) {
    } else {
        ESP_LOGE(TAG, "Failed to read illuminance");
    }

    for (const auto& icon : instance->get_icons_vector()) {
        update_status(icon, intensity_status);
    }

    esp_event_loop_handle_t event_loop_handle = instance->get_event_loop_handle();
    if (event_loop_handle) {
        light_sensor_data_t sensor_data {
            .illuminance = illuminance,
            .intensity_status = intensity_status
        };

        esp_event_post_to(event_loop_handle, LIGHT_SENSOR_EVENTS, LightSensorUpdate, &sensor_data, sizeof(light_sensor_data_t), 100 / portTICK_PERIOD_MS);
    }

}

LightSensor::LightSensor()
{
    ESP_LOGI(TAG, "Initializing...");

    m_is_present = false;
    m_event_loop_handle = nullptr;
    m_periodic_timer = nullptr;

    i2c_master_bus_handle_t i2c_master_bus_handle;
    ESP_ERROR_CHECK(i2c_master_get_bus_handle(0, &i2c_master_bus_handle));

    if (i2c_master_probe(i2c_master_bus_handle, CONFIG_LIGHT_SENSOR_I2C_ADDR, 100) != ESP_OK)
    {
        ESP_LOGW(TAG, "BH1750 not found at address 0x%02X", CONFIG_LIGHT_SENSOR_I2C_ADDR);
        return;
    }

    ESP_LOGI(TAG, "Found BH1750 at address 0x%02X", CONFIG_LIGHT_SENSOR_I2C_ADDR);
    m_is_present = true;

    m_semaphore_handle = xSemaphoreCreateMutex();
    if (m_semaphore_handle == nullptr)
    {
        ESP_LOGE(TAG, "Failed to create semaphore");
        m_is_present = false;
        return;
    }

    xSemaphoreTake(m_semaphore_handle, 0);

    ESP_LOGI(TAG, "Found device at 0x%x", CONFIG_LIGHT_SENSOR_I2C_ADDR);
    i2c_device_config_t i2c_device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CONFIG_LIGHT_SENSOR_I2C_ADDR,
        .scl_speed_hz = 100000,
        .scl_wait_us = 0,
        .flags = {}
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_master_bus_handle, &i2c_device_config, &m_i2c_device_handle));

    xSemaphoreGive(m_semaphore_handle);

    ESP_ERROR_CHECK(power_on());
    ESP_ERROR_CHECK(set_measure_mode(measure_mode_t::ContinuouslyHResolutionMode));

    if (CONFIG_LIGHT_SENSOR_POLLING_INTERVAL_SEC != 0) {
        ESP_LOGI(TAG, "Light sensor polling interval set to %d Second(s).", CONFIG_LIGHT_SENSOR_POLLING_INTERVAL_SEC);
        esp_timer_create_args_t timer_args = {
            .callback = timer_callback,
            .arg = this,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "LightSensorTimer",
            .skip_unhandled_events = false
        };

        esp_timer_create(&timer_args, &m_periodic_timer);
        esp_timer_start_periodic(m_periodic_timer, CONFIG_LIGHT_SENSOR_POLLING_INTERVAL_SEC * 1000000);
    } else {
        ESP_LOGI(TAG, "Light sensor polling disabled");
    }
}

LightSensor::~LightSensor()
{
}

esp_err_t LightSensor::reset()
{
    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    uint8_t data = BH1750_POWER_RESET;
    esp_err_t result = i2c_master_transmit(m_i2c_device_handle, &data, 1, -1);
    xSemaphoreGive(m_semaphore_handle);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to power down light sensor");

    return ESP_OK;
}

esp_err_t LightSensor::power_down()
{
    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    uint8_t data = BH1750_POWER_DOWN;
    esp_err_t result = i2c_master_transmit(m_i2c_device_handle, &data, 1, -1);
    xSemaphoreGive(m_semaphore_handle);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to power down light sensor");

    return ESP_OK;
}

esp_err_t LightSensor::power_on()
{
    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    uint8_t data = BH1750_POWER_ON;
    esp_err_t result = i2c_master_transmit(m_i2c_device_handle, &data, 1, -1);
    xSemaphoreGive(m_semaphore_handle);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to power on light sensor");

    return ESP_OK;
}

esp_err_t LightSensor::set_measure_mode(const measure_mode_t measure_mode)
{
    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    uint8_t mode = static_cast<uint8_t>(measure_mode);
    esp_err_t result = i2c_master_transmit(m_i2c_device_handle, &mode, 1, -1);
    xSemaphoreGive(m_semaphore_handle);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to set measure mode");

    return ESP_OK;
}

esp_err_t LightSensor::get_illuminance(uint16_t *illuminance, intensity_status_t *intensity_status)
{
    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    uint8_t buffer[2] = {0};
    esp_err_t result = i2c_master_receive(m_i2c_device_handle, buffer, 2, -1);
    if (result != ESP_OK) {
        xSemaphoreGive(m_semaphore_handle);
        ESP_RETURN_ON_ERROR(result, TAG, "Failed to read illuminance");
    }

    *illuminance = (buffer[0] << 8) | buffer[1];

    if (intensity_status != nullptr) {
        if (*illuminance <= UINT16_MAX / 5) {  // 0-20%
            *intensity_status = Lowest;
        } else if (*illuminance <= (UINT16_MAX * 2) / 5) {  // 20-40%
            *intensity_status = Low;
        } else if (*illuminance <= (UINT16_MAX * 3) / 5) {  // 40-60%
            *intensity_status = Medium;
        } else if (*illuminance <= (UINT16_MAX * 4) / 5) {  // 60-80%
            *intensity_status = High;
        } else {                                            // 80-100%
            *intensity_status = Highest;
        }
    }

    xSemaphoreGive(m_semaphore_handle);

    return ESP_OK;
}

esp_err_t LightSensor::set_measure_time(const uint8_t measure_time)
{
    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    esp_err_t result = ESP_OK;
    uint8_t data[2] = {0x40, 0x60}; // constant part of the the MTreg
    data[0] |= measure_time >> 5;
    data[1] |= measure_time & 0x1F;
    result = i2c_master_transmit(m_i2c_device_handle, data, sizeof(data), -1);
    xSemaphoreGive(m_semaphore_handle);
    ESP_RETURN_ON_ERROR(result, TAG, "Failed to set measure time");

    return ESP_OK;
}

esp_err_t LightSensor::add_lv_obj_icon(lv_obj_t *lv_obj_icon)
{
    if (lv_obj_icon == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    m_icons.push_back(lv_obj_icon);

    return ESP_OK;
}

esp_err_t LightSensor::register_event_handler(esp_event_handler_t event_handler, void* handler_arg)
{
    if (!event_handler) {
        return ESP_ERR_INVALID_ARG;
    }
    
    return esp_event_handler_register_with(m_event_loop_handle, LIGHT_SENSOR_EVENTS, ESP_EVENT_ANY_ID, event_handler, handler_arg);
}

esp_err_t LightSensor::unregister_event_handler(esp_event_handler_t event_handler)
{
    if (!event_handler) {
        return ESP_ERR_INVALID_ARG;
    }
    
    return esp_event_handler_unregister_with(m_event_loop_handle, LIGHT_SENSOR_EVENTS, ESP_EVENT_ANY_ID, event_handler);
}



