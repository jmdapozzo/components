#include "rtc.hpp"
#include <esp_log.h>
#include <esp_err.h>
#include <esp_check.h>
#include <string.h>

using namespace macdap;

// MCP7940 I2C Address
#define MCP7940_I2CADDR_DEFAULT     0x6F

// MCP7940 Register Map
#define MCP7940_REG_RTCSEC          0x00    // Seconds register
#define MCP7940_REG_RTCMIN          0x01    // Minutes register
#define MCP7940_REG_RTCHOUR         0x02    // Hours register
#define MCP7940_REG_RTCWKDAY        0x03    // Day of week register
#define MCP7940_REG_RTCDATE         0x04    // Date register
#define MCP7940_REG_RTCMTH          0x05    // Month register
#define MCP7940_REG_RTCYEAR         0x06    // Year register
#define MCP7940_REG_CONTROL         0x07    // Control register
#define MCP7940_REG_OSCTRIM         0x08    // Oscillator trim register
#define MCP7940_REG_ALM0SEC         0x0A    // Alarm 0 Seconds
#define MCP7940_REG_ALM0MIN         0x0B    // Alarm 0 Minutes
#define MCP7940_REG_ALM0HOUR        0x0C    // Alarm 0 Hours
#define MCP7940_REG_ALM0WKDAY       0x0D    // Alarm 0 Day of Week
#define MCP7940_REG_ALM0DATE        0x0E    // Alarm 0 Date
#define MCP7940_REG_ALM0MTH         0x0F    // Alarm 0 Month
#define MCP7940_REG_ALM1SEC         0x11    // Alarm 1 Seconds
#define MCP7940_REG_ALM1MIN         0x12    // Alarm 1 Minutes
#define MCP7940_REG_ALM1HOUR        0x13    // Alarm 1 Hours
#define MCP7940_REG_ALM1WKDAY       0x14    // Alarm 1 Day of Week
#define MCP7940_REG_ALM1DATE        0x15    // Alarm 1 Date
#define MCP7940_REG_ALM1MTH         0x16    // Alarm 1 Month
#define MCP7940_REG_PWRDNMIN        0x18    // Power-Down Minutes
#define MCP7940_REG_PWRDNHOUR       0x19    // Power-Down Hours
#define MCP7940_REG_PWRDNDATE       0x1A    // Power-Down Date
#define MCP7940_REG_PWRDNMTH        0x1B    // Power-Down Month
#define MCP7940_REG_PWRUPMIN        0x1C    // Power-Up Minutes
#define MCP7940_REG_PWRUPHOUR       0x1D    // Power-Up Hours
#define MCP7940_REG_PWRUPDATE       0x1E    // Power-Up Date
#define MCP7940_REG_PWRUPMTH        0x1F    // Power-Up Month
#define MCP7940_REG_SRAM_START      0x20    // SRAM start address
#define MCP7940_REG_SRAM_END        0x5F    // SRAM end address (64 bytes)

// Bit definitions
#define MCP7940_BIT_ST              0x80    // Start/Stop bit (Seconds register)
#define MCP7940_BIT_12_24           0x40    // 12/24 hour mode (Hours register)
#define MCP7940_BIT_VBATEN          0x08    // Battery Enable bit (Day register)
#define MCP7940_BIT_OSCRUN          0x20    // Oscillator Running bit (Day register)
#define MCP7940_BIT_PWRFAIL         0x10    // Power Failure bit (Day register)
#define MCP7940_BIT_LPYR            0x20    // Leap Year bit (Month register)
#define MCP7940_BIT_OUT             0x80    // Output control bit (Control register)
#define MCP7940_BIT_SQWEN           0x40    // Square Wave Enable (Control register)
#define MCP7940_BIT_ALM1EN          0x20    // Alarm 1 Enable (Control register)
#define MCP7940_BIT_ALM0EN          0x10    // Alarm 0 Enable (Control register)
#define MCP7940_BIT_EXTOSC          0x08    // External Oscillator (Control register)
#define MCP7940_BIT_CRSTRIM         0x04    // Coarse Trim Mode (Control register)
#define MCP7940_BIT_SQWFS1          0x02    // Square Wave Frequency Select bit 1
#define MCP7940_BIT_SQWFS0          0x01    // Square Wave Frequency Select bit 0

static const char *TAG = "rtc";

ESP_EVENT_DEFINE_BASE(RTC_EVENTS);

RTC::RTC()
{
    ESP_LOGI(TAG, "Initializing MCP7940 RTC...");

    m_is_present = false;
    m_battery_enabled = false;
    m_event_loop_handle = nullptr;

    // Get I2C master bus handle
    i2c_master_bus_handle_t i2c_master_bus_handle;
    ESP_ERROR_CHECK(i2c_master_get_bus_handle(CONFIG_RTC_I2C_PORT_NUM, &i2c_master_bus_handle));

    // Probe for the RTC device
    if (i2c_master_probe(i2c_master_bus_handle, CONFIG_RTC_I2C_ADDR, 100) != ESP_OK)
    {
        ESP_LOGW(TAG, "MCP7940 not found at address 0x%02X", CONFIG_RTC_I2C_ADDR);
        return;
    }

    ESP_LOGI(TAG, "Found MCP7940 at address 0x%02X", CONFIG_RTC_I2C_ADDR);
    m_is_present = true;

    // Create semaphore for thread safety
    m_semaphore_handle = xSemaphoreCreateMutex();
    if (m_semaphore_handle == nullptr)
    {
        ESP_LOGE(TAG, "Failed to create semaphore");
        m_is_present = false;
        return;
    }

    xSemaphoreTake(m_semaphore_handle, 0);

    // Configure I2C device
    i2c_device_config_t i2c_device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CONFIG_RTC_I2C_ADDR,
        .scl_speed_hz = 100000,  // 100 kHz
        .scl_wait_us = 0,
        .flags = {}
    };

    if (i2c_master_bus_add_device(i2c_master_bus_handle, &i2c_device_config, &m_i2c_device_handle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add I2C device");
        xSemaphoreGive(m_semaphore_handle);
        m_is_present = false;
        return;
    }

    xSemaphoreGive(m_semaphore_handle);

    // Check if battery backup is enabled
    uint8_t wkday;
    if (read_register(MCP7940_REG_RTCWKDAY, &wkday, 1) == ESP_OK)
    {
        m_battery_enabled = (wkday & MCP7940_BIT_VBATEN) != 0;
        ESP_LOGI(TAG, "Battery backup: %s", m_battery_enabled ? "enabled" : "disabled");
    }

    // Check if oscillator is running
    bool running;
    if (is_running(&running) == ESP_OK)
    {
        ESP_LOGI(TAG, "Oscillator: %s", running ? "running" : "stopped");
    }

    ESP_LOGI(TAG, "Initialization complete");
}

RTC::~RTC()
{
    if (m_semaphore_handle != nullptr)
    {
        vSemaphoreDelete(m_semaphore_handle);
    }
}

// Internal helper methods
esp_err_t RTC::read_register(uint8_t reg_addr, uint8_t *data, size_t len)
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t write_buf[1] = {reg_addr};
    return i2c_master_transmit_receive(m_i2c_device_handle, write_buf, sizeof(write_buf), data, len, -1);
}

esp_err_t RTC::write_register(uint8_t reg_addr, const uint8_t *data, size_t len)
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t write_buf[len + 1];
    write_buf[0] = reg_addr;
    memcpy(&write_buf[1], data, len);

    return i2c_master_transmit(m_i2c_device_handle, write_buf, sizeof(write_buf), -1);
}

uint8_t RTC::bcd_to_dec(uint8_t bcd)
{
    return (bcd / 16 * 10) + (bcd % 16);
}

uint8_t RTC::dec_to_bcd(uint8_t dec)
{
    return (dec / 10 * 16) + (dec % 10);
}

// Core RTC functions
esp_err_t RTC::get_time(struct tm *time_info)
{
    if (!m_is_present || time_info == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    uint8_t buffer[7];
    esp_err_t result = read_register(MCP7940_REG_RTCSEC, buffer, 7);

    if (result == ESP_OK)
    {
        time_info->tm_sec = bcd_to_dec(buffer[0] & 0x7F);  // Mask out ST bit
        time_info->tm_min = bcd_to_dec(buffer[1] & 0x7F);
        time_info->tm_hour = bcd_to_dec(buffer[2] & 0x3F); // Mask out 12/24 bit
        time_info->tm_wday = (buffer[3] & 0x07) - 1;       // MCP7940 uses 1-7, tm uses 0-6
        time_info->tm_mday = bcd_to_dec(buffer[4] & 0x3F);
        time_info->tm_mon = bcd_to_dec(buffer[5] & 0x1F) - 1; // MCP7940 uses 1-12, tm uses 0-11
        time_info->tm_year = bcd_to_dec(buffer[6]) + 100;  // tm_year is years since 1900
        time_info->tm_isdst = -1;  // Unknown
    }

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t RTC::set_time(const struct tm *time_info)
{
    if (!m_is_present || time_info == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    // Read current registers to preserve flags
    uint8_t current[7];
    esp_err_t result = read_register(MCP7940_REG_RTCSEC, current, 7);
    if (result != ESP_OK)
    {
        xSemaphoreGive(m_semaphore_handle);
        return result;
    }

    // Prepare data to write
    uint8_t buffer[7];
    buffer[0] = dec_to_bcd(time_info->tm_sec) | (current[0] & MCP7940_BIT_ST); // Preserve ST bit
    buffer[1] = dec_to_bcd(time_info->tm_min);
    buffer[2] = dec_to_bcd(time_info->tm_hour); // 24-hour format
    buffer[3] = (time_info->tm_wday + 1) | (current[3] & (MCP7940_BIT_VBATEN | MCP7940_BIT_OSCRUN | MCP7940_BIT_PWRFAIL)); // Preserve flags
    buffer[4] = dec_to_bcd(time_info->tm_mday);
    buffer[5] = dec_to_bcd(time_info->tm_mon + 1); // tm uses 0-11, MCP7940 uses 1-12
    buffer[6] = dec_to_bcd(time_info->tm_year % 100); // Only last 2 digits of year

    result = write_register(MCP7940_REG_RTCSEC, buffer, 7);

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t RTC::is_running(bool *running)
{
    if (!m_is_present || running == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    uint8_t wkday;
    esp_err_t result = read_register(MCP7940_REG_RTCWKDAY, &wkday, 1);

    if (result == ESP_OK)
    {
        *running = (wkday & MCP7940_BIT_OSCRUN) != 0;
    }

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t RTC::start()
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    uint8_t sec;
    esp_err_t result = read_register(MCP7940_REG_RTCSEC, &sec, 1);

    if (result == ESP_OK)
    {
        sec |= MCP7940_BIT_ST;  // Set the ST bit
        result = write_register(MCP7940_REG_RTCSEC, &sec, 1);
    }

    xSemaphoreGive(m_semaphore_handle);

    if (result == ESP_OK)
    {
        ESP_LOGI(TAG, "Oscillator started");
    }

    return result;
}

esp_err_t RTC::stop()
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    uint8_t sec;
    esp_err_t result = read_register(MCP7940_REG_RTCSEC, &sec, 1);

    if (result == ESP_OK)
    {
        sec &= ~MCP7940_BIT_ST;  // Clear the ST bit
        result = write_register(MCP7940_REG_RTCSEC, &sec, 1);
    }

    xSemaphoreGive(m_semaphore_handle);

    if (result == ESP_OK)
    {
        ESP_LOGI(TAG, "Oscillator stopped");
    }

    return result;
}

// System time synchronization
esp_err_t RTC::sync_from_system()
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    esp_err_t result = set_time(&timeinfo);

    if (result == ESP_OK)
    {
        ESP_LOGI(TAG, "Synced RTC from system time: %04d-%02d-%02d %02d:%02d:%02d",
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }

    return result;
}

esp_err_t RTC::sync_to_system()
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    struct tm timeinfo;
    esp_err_t result = get_time(&timeinfo);

    if (result == ESP_OK)
    {
        struct timeval tv;
        tv.tv_sec = mktime(&timeinfo);
        tv.tv_usec = 0;

        if (settimeofday(&tv, nullptr) == 0)
        {
            ESP_LOGI(TAG, "Synced system time from RTC: %04d-%02d-%02d %02d:%02d:%02d",
                     timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        }
        else
        {
            result = ESP_FAIL;
        }
    }

    return result;
}

// Battery backup
esp_err_t RTC::enable_battery_backup(bool enable)
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    uint8_t wkday;
    esp_err_t result = read_register(MCP7940_REG_RTCWKDAY, &wkday, 1);

    if (result == ESP_OK)
    {
        if (enable)
        {
            wkday |= MCP7940_BIT_VBATEN;
        }
        else
        {
            wkday &= ~MCP7940_BIT_VBATEN;
        }

        result = write_register(MCP7940_REG_RTCWKDAY, &wkday, 1);

        if (result == ESP_OK)
        {
            m_battery_enabled = enable;
            ESP_LOGI(TAG, "Battery backup %s", enable ? "enabled" : "disabled");
        }
    }

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t RTC::is_battery_enabled(bool *enabled)
{
    if (!m_is_present || enabled == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *enabled = m_battery_enabled;
    return ESP_OK;
}

// Power fail/restore timestamps
esp_err_t RTC::get_power_down_timestamp(struct tm *time_info)
{
    if (!m_is_present || time_info == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    uint8_t buffer[4];
    esp_err_t result = read_register(MCP7940_REG_PWRDNMIN, buffer, 4);

    if (result == ESP_OK)
    {
        time_info->tm_min = bcd_to_dec(buffer[0] & 0x7F);
        time_info->tm_hour = bcd_to_dec(buffer[1] & 0x3F);
        time_info->tm_mday = bcd_to_dec(buffer[2] & 0x3F);
        time_info->tm_mon = bcd_to_dec(buffer[3] & 0x1F) - 1;
        time_info->tm_wday = (buffer[3] >> 5) & 0x07;
    }

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t RTC::get_power_up_timestamp(struct tm *time_info)
{
    if (!m_is_present || time_info == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    uint8_t buffer[4];
    esp_err_t result = read_register(MCP7940_REG_PWRUPMIN, buffer, 4);

    if (result == ESP_OK)
    {
        time_info->tm_min = bcd_to_dec(buffer[0] & 0x7F);
        time_info->tm_hour = bcd_to_dec(buffer[1] & 0x3F);
        time_info->tm_mday = bcd_to_dec(buffer[2] & 0x3F);
        time_info->tm_mon = bcd_to_dec(buffer[3] & 0x1F) - 1;
        time_info->tm_wday = (buffer[3] >> 5) & 0x07;
    }

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t RTC::has_lost_power(bool *lost_power)
{
    if (!m_is_present || lost_power == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    uint8_t wkday;
    esp_err_t result = read_register(MCP7940_REG_RTCWKDAY, &wkday, 1);

    if (result == ESP_OK)
    {
        *lost_power = (wkday & MCP7940_BIT_PWRFAIL) != 0;
    }

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t RTC::clear_power_fail_flag()
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    uint8_t wkday;
    esp_err_t result = read_register(MCP7940_REG_RTCWKDAY, &wkday, 1);

    if (result == ESP_OK)
    {
        wkday &= ~MCP7940_BIT_PWRFAIL;
        result = write_register(MCP7940_REG_RTCWKDAY, &wkday, 1);
    }

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

// Alarm functions (simplified - full implementation would be more complex)
esp_err_t RTC::set_alarm(alarm_t alarm, const struct tm *time_info, alarm_match_t match_mode)
{
    if (!m_is_present || time_info == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    uint8_t base_reg = (alarm == Alarm1) ? MCP7940_REG_ALM1SEC : MCP7940_REG_ALM0SEC;

    uint8_t buffer[6];
    buffer[0] = dec_to_bcd(time_info->tm_sec);
    buffer[1] = dec_to_bcd(time_info->tm_min);
    buffer[2] = dec_to_bcd(time_info->tm_hour);
    buffer[3] = (time_info->tm_wday + 1) | (match_mode << 4); // Match mode in upper bits
    buffer[4] = dec_to_bcd(time_info->tm_mday);
    buffer[5] = dec_to_bcd(time_info->tm_mon + 1);

    esp_err_t result = write_register(base_reg, buffer, 6);

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t RTC::enable_alarm(alarm_t alarm, bool enable)
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    uint8_t control;
    esp_err_t result = read_register(MCP7940_REG_CONTROL, &control, 1);

    if (result == ESP_OK)
    {
        uint8_t alarm_bit = (alarm == Alarm1) ? MCP7940_BIT_ALM1EN : MCP7940_BIT_ALM0EN;

        if (enable)
        {
            control |= alarm_bit;
        }
        else
        {
            control &= ~alarm_bit;
        }

        result = write_register(MCP7940_REG_CONTROL, &control, 1);
    }

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t RTC::is_alarm_triggered(alarm_t alarm, bool *triggered)
{
    if (!m_is_present || triggered == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    uint8_t base_reg = (alarm == Alarm1) ? MCP7940_REG_ALM1WKDAY : MCP7940_REG_ALM0WKDAY;
    uint8_t wkday;
    esp_err_t result = read_register(base_reg, &wkday, 1);

    if (result == ESP_OK)
    {
        *triggered = (wkday & 0x08) != 0;  // Alarm IF bit
    }

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t RTC::clear_alarm_flag(alarm_t alarm)
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    uint8_t base_reg = (alarm == Alarm1) ? MCP7940_REG_ALM1WKDAY : MCP7940_REG_ALM0WKDAY;
    uint8_t wkday;
    esp_err_t result = read_register(base_reg, &wkday, 1);

    if (result == ESP_OK)
    {
        wkday &= ~0x08;  // Clear alarm IF bit
        result = write_register(base_reg, &wkday, 1);
    }

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

// Square wave output
esp_err_t RTC::enable_square_wave(bool enable)
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    uint8_t control;
    esp_err_t result = read_register(MCP7940_REG_CONTROL, &control, 1);

    if (result == ESP_OK)
    {
        if (enable)
        {
            control |= MCP7940_BIT_SQWEN;
        }
        else
        {
            control &= ~MCP7940_BIT_SQWEN;
        }

        result = write_register(MCP7940_REG_CONTROL, &control, 1);
    }

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t RTC::set_square_wave_frequency(uint8_t freq)
{
    if (!m_is_present || freq > 3)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    uint8_t control;
    esp_err_t result = read_register(MCP7940_REG_CONTROL, &control, 1);

    if (result == ESP_OK)
    {
        control &= ~(MCP7940_BIT_SQWFS1 | MCP7940_BIT_SQWFS0);  // Clear frequency bits
        control |= (freq & 0x03);  // Set new frequency

        result = write_register(MCP7940_REG_CONTROL, &control, 1);
    }

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

// SRAM access
esp_err_t RTC::read_sram(uint8_t address, uint8_t *data, size_t len)
{
    if (!m_is_present || data == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (address + len > 64)  // SRAM is 64 bytes
    {
        return ESP_ERR_INVALID_SIZE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    esp_err_t result = read_register(MCP7940_REG_SRAM_START + address, data, len);

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t RTC::write_sram(uint8_t address, const uint8_t *data, size_t len)
{
    if (!m_is_present || data == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (address + len > 64)  // SRAM is 64 bytes
    {
        return ESP_ERR_INVALID_SIZE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    esp_err_t result = write_register(MCP7940_REG_SRAM_START + address, data, len);

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

// Event handling
esp_err_t RTC::register_event_handler(esp_event_handler_t event_handler, void* handler_arg)
{
    if (m_event_loop_handle == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }

    return esp_event_handler_register_with(
        m_event_loop_handle,
        RTC_EVENTS,
        ESP_EVENT_ANY_ID,
        event_handler,
        handler_arg
    );
}

esp_err_t RTC::unregister_event_handler(esp_event_handler_t event_handler)
{
    if (m_event_loop_handle == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }

    return esp_event_handler_unregister_with(
        m_event_loop_handle,
        RTC_EVENTS,
        ESP_EVENT_ANY_ID,
        event_handler
    );
}
