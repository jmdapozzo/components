#include "eeprom.hpp"
#include <esp_log.h>
#include <esp_err.h>
#include <esp_check.h>
#include <string.h>
#include <freertos/task.h>

using namespace macdap;

// 24LC08B I2C Base Address (blocks at 0x50, 0x51, 0x52, 0x53)
#define EEPROM_I2C_BASE_ADDR    0x50

static const char *TAG = "eeprom";

EEPROM::EEPROM()
{
    ESP_LOGI(TAG, "Initializing 24LC08B EEPROM...");

    m_is_present = false;
    m_base_address = CONFIG_I2C_EEPROM_BASE_ADDR;

    // Initialize device handles to NULL
    for (int i = 0; i < EEPROM_NUM_BLOCKS; i++) {
        m_i2c_device_handles[i] = nullptr;
    }

    // Get I2C master bus handle
    i2c_master_bus_handle_t i2c_master_bus_handle;
    ESP_ERROR_CHECK(i2c_master_get_bus_handle(CONFIG_I2C_PORT_NUM, &i2c_master_bus_handle));

    // Probe for the EEPROM at base address
    if (i2c_master_probe(i2c_master_bus_handle, m_base_address, 100) != ESP_OK)
    {
        ESP_LOGW(TAG, "24LC08B not found at base address 0x%02X", m_base_address);
        return;
    }

    ESP_LOGI(TAG, "Found 24LC08B at base address 0x%02X", m_base_address);
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

    // Add I2C devices for all 4 blocks
    for (uint8_t block = 0; block < EEPROM_NUM_BLOCKS; block++)
    {
        i2c_device_config_t i2c_device_config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = static_cast<uint16_t>(m_base_address + block),
            .scl_speed_hz = 400000,  // 400 kHz (24LC08B supports up to 400kHz)
            .scl_wait_us = 0,
            .flags = {}
        };

        if (i2c_master_bus_add_device(i2c_master_bus_handle, &i2c_device_config, &m_i2c_device_handles[block]) != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to add I2C device for block %d", block);
            xSemaphoreGive(m_semaphore_handle);
            m_is_present = false;
            return;
        }
    }

    xSemaphoreGive(m_semaphore_handle);

    ESP_LOGI(TAG, "Initialization complete: %d bytes (%d blocks x %d bytes)",
             EEPROM_TOTAL_SIZE, EEPROM_NUM_BLOCKS, EEPROM_BLOCK_SIZE);
}

EEPROM::~EEPROM()
{
    if (m_semaphore_handle != nullptr)
    {
        vSemaphoreDelete(m_semaphore_handle);
    }
}

// Helper methods
uint8_t EEPROM::get_block_number(uint16_t address)
{
    return (address / EEPROM_BLOCK_SIZE) % EEPROM_NUM_BLOCKS;
}

uint8_t EEPROM::get_block_address(uint16_t address)
{
    return address % EEPROM_BLOCK_SIZE;
}

uint8_t EEPROM::get_device_address(uint16_t address)
{
    return m_base_address + get_block_number(address);
}

void EEPROM::wait_write_cycle()
{
    vTaskDelay(pdMS_TO_TICKS(EEPROM_WRITE_DELAY_MS));
}

// Internal read operation
esp_err_t EEPROM::read_internal(uint16_t address, uint8_t *data, size_t length)
{
    if (!m_is_present || data == nullptr || length == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (address + length > EEPROM_TOTAL_SIZE)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    size_t bytes_read = 0;

    while (bytes_read < length)
    {
        uint16_t current_address = address + bytes_read;
        uint8_t block_num = get_block_number(current_address);
        uint8_t block_addr = get_block_address(current_address);

        // Calculate how many bytes to read from this block
        size_t bytes_remaining = length - bytes_read;
        size_t bytes_in_block = EEPROM_BLOCK_SIZE - block_addr;
        size_t bytes_to_read = (bytes_remaining < bytes_in_block) ? bytes_remaining : bytes_in_block;

        // Read from this block
        uint8_t addr_byte = block_addr;
        esp_err_t result = i2c_master_transmit_receive(
            m_i2c_device_handles[block_num],
            &addr_byte, 1,
            &data[bytes_read], bytes_to_read,
            -1
        );

        if (result != ESP_OK)
        {
            return result;
        }

        bytes_read += bytes_to_read;
    }

    return ESP_OK;
}

// Internal write operation (byte-by-byte, slow but safe)
esp_err_t EEPROM::write_internal(uint16_t address, const uint8_t *data, size_t length)
{
    if (!m_is_present || data == nullptr || length == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (address + length > EEPROM_TOTAL_SIZE)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    for (size_t i = 0; i < length; i++)
    {
        uint16_t current_address = address + i;
        uint8_t block_num = get_block_number(current_address);
        uint8_t block_addr = get_block_address(current_address);

        uint8_t write_data[2] = {block_addr, data[i]};

        esp_err_t result = i2c_master_transmit(
            m_i2c_device_handles[block_num],
            write_data, 2,
            -1
        );

        if (result != ESP_OK)
        {
            return result;
        }

        // Wait for write cycle to complete
        wait_write_cycle();
    }

    return ESP_OK;
}

// Page write (up to 16 bytes, much faster than byte-by-byte)
esp_err_t EEPROM::write_page_internal(uint16_t address, const uint8_t *data, size_t length)
{
    if (!m_is_present || data == nullptr || length == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (length > EEPROM_PAGE_SIZE)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    if (address + length > EEPROM_TOTAL_SIZE)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    // Check if write crosses page boundary
    uint16_t page_start = (address / EEPROM_PAGE_SIZE) * EEPROM_PAGE_SIZE;
    if (address + length > page_start + EEPROM_PAGE_SIZE)
    {
        return ESP_ERR_INVALID_ARG;  // Cannot cross page boundary
    }

    uint8_t block_num = get_block_number(address);
    uint8_t block_addr = get_block_address(address);

    // Prepare write buffer: address + data
    uint8_t write_buffer[EEPROM_PAGE_SIZE + 1];
    write_buffer[0] = block_addr;
    memcpy(&write_buffer[1], data, length);

    esp_err_t result = i2c_master_transmit(
        m_i2c_device_handles[block_num],
        write_buffer, length + 1,
        -1
    );

    if (result != ESP_OK)
    {
        return result;
    }

    // Wait for write cycle to complete
    wait_write_cycle();

    return ESP_OK;
}

// Public API - Basic operations
esp_err_t EEPROM::read_byte(uint16_t address, uint8_t *data)
{
    if (!m_is_present || data == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    esp_err_t result = read_internal(address, data, 1);
    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t EEPROM::write_byte(uint16_t address, uint8_t data)
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    esp_err_t result = write_internal(address, &data, 1);
    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t EEPROM::read(uint16_t address, uint8_t *data, size_t length)
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    esp_err_t result = read_internal(address, data, length);
    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t EEPROM::write(uint16_t address, const uint8_t *data, size_t length)
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    // Use page writes for efficiency
    size_t bytes_written = 0;

    while (bytes_written < length)
    {
        uint16_t current_address = address + bytes_written;
        size_t bytes_remaining = length - bytes_written;

        // Calculate page-aligned write
        uint16_t page_offset = current_address % EEPROM_PAGE_SIZE;
        size_t bytes_in_page = EEPROM_PAGE_SIZE - page_offset;
        size_t bytes_to_write = (bytes_remaining < bytes_in_page) ? bytes_remaining : bytes_in_page;

        esp_err_t result = write_page_internal(current_address, &data[bytes_written], bytes_to_write);
        if (result != ESP_OK)
        {
            xSemaphoreGive(m_semaphore_handle);
            return result;
        }

        bytes_written += bytes_to_write;
    }

    xSemaphoreGive(m_semaphore_handle);
    return ESP_OK;
}

esp_err_t EEPROM::write_page(uint16_t address, const uint8_t *data, size_t length)
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (length > EEPROM_PAGE_SIZE)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    esp_err_t result = write_page_internal(address, data, length);
    xSemaphoreGive(m_semaphore_handle);

    return result;
}

// Utility functions
esp_err_t EEPROM::clear(uint16_t address, size_t length)
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t zero_buffer[EEPROM_PAGE_SIZE];
    memset(zero_buffer, 0x00, EEPROM_PAGE_SIZE);

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    size_t bytes_cleared = 0;

    while (bytes_cleared < length)
    {
        uint16_t current_address = address + bytes_cleared;
        size_t bytes_remaining = length - bytes_cleared;

        uint16_t page_offset = current_address % EEPROM_PAGE_SIZE;
        size_t bytes_in_page = EEPROM_PAGE_SIZE - page_offset;
        size_t bytes_to_clear = (bytes_remaining < bytes_in_page) ? bytes_remaining : bytes_in_page;

        esp_err_t result = write_page_internal(current_address, zero_buffer, bytes_to_clear);
        if (result != ESP_OK)
        {
            xSemaphoreGive(m_semaphore_handle);
            return result;
        }

        bytes_cleared += bytes_to_clear;
    }

    xSemaphoreGive(m_semaphore_handle);
    return ESP_OK;
}

esp_err_t EEPROM::clear_all()
{
    ESP_LOGI(TAG, "Clearing entire EEPROM (%d bytes)...", EEPROM_TOTAL_SIZE);
    esp_err_t result = clear(0, EEPROM_TOTAL_SIZE);
    if (result == ESP_OK)
    {
        ESP_LOGI(TAG, "EEPROM cleared successfully");
    }
    return result;
}

esp_err_t EEPROM::fill(uint16_t address, size_t length, uint8_t value)
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t fill_buffer[EEPROM_PAGE_SIZE];
    memset(fill_buffer, value, EEPROM_PAGE_SIZE);

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    size_t bytes_filled = 0;

    while (bytes_filled < length)
    {
        uint16_t current_address = address + bytes_filled;
        size_t bytes_remaining = length - bytes_filled;

        uint16_t page_offset = current_address % EEPROM_PAGE_SIZE;
        size_t bytes_in_page = EEPROM_PAGE_SIZE - page_offset;
        size_t bytes_to_fill = (bytes_remaining < bytes_in_page) ? bytes_remaining : bytes_in_page;

        esp_err_t result = write_page_internal(current_address, fill_buffer, bytes_to_fill);
        if (result != ESP_OK)
        {
            xSemaphoreGive(m_semaphore_handle);
            return result;
        }

        bytes_filled += bytes_to_fill;
    }

    xSemaphoreGive(m_semaphore_handle);
    return ESP_OK;
}

// Block operations
esp_err_t EEPROM::read_block(uint8_t block_num, uint8_t *data)
{
    if (block_num >= EEPROM_NUM_BLOCKS || data == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t address = block_num * EEPROM_BLOCK_SIZE;
    return read(address, data, EEPROM_BLOCK_SIZE);
}

esp_err_t EEPROM::write_block(uint8_t block_num, const uint8_t *data)
{
    if (block_num >= EEPROM_NUM_BLOCKS || data == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t address = block_num * EEPROM_BLOCK_SIZE;
    return write(address, data, EEPROM_BLOCK_SIZE);
}

esp_err_t EEPROM::clear_block(uint8_t block_num)
{
    if (block_num >= EEPROM_NUM_BLOCKS)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t address = block_num * EEPROM_BLOCK_SIZE;
    ESP_LOGI(TAG, "Clearing block %d...", block_num);
    return clear(address, EEPROM_BLOCK_SIZE);
}

// Verification
esp_err_t EEPROM::verify(uint16_t address, const uint8_t *data, size_t length)
{
    if (!m_is_present || data == nullptr || length == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (address + length > EEPROM_TOTAL_SIZE)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    uint8_t read_buffer[64];  // Read in chunks
    size_t bytes_verified = 0;

    while (bytes_verified < length)
    {
        size_t bytes_remaining = length - bytes_verified;
        size_t bytes_to_verify = (bytes_remaining < sizeof(read_buffer)) ? bytes_remaining : sizeof(read_buffer);

        esp_err_t result = read_internal(address + bytes_verified, read_buffer, bytes_to_verify);
        if (result != ESP_OK)
        {
            xSemaphoreGive(m_semaphore_handle);
            return result;
        }

        if (memcmp(&data[bytes_verified], read_buffer, bytes_to_verify) != 0)
        {
            xSemaphoreGive(m_semaphore_handle);
            ESP_LOGE(TAG, "Verification failed at address %d", address + bytes_verified);
            return ESP_ERR_INVALID_CRC;
        }

        bytes_verified += bytes_to_verify;
    }

    xSemaphoreGive(m_semaphore_handle);
    return ESP_OK;
}
