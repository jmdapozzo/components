#pragma once

#include "esp_err.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <driver/i2c_master.h>
#include <cstdint>
#include <cstddef>

namespace macdap
{
    // 24LC08B EEPROM specifications
    static constexpr size_t EEPROM_TOTAL_SIZE = 1024;      // 8Kbit = 1024 bytes
    static constexpr size_t EEPROM_BLOCK_SIZE = 256;       // 4 blocks of 256 bytes
    static constexpr size_t EEPROM_PAGE_SIZE = 16;         // 16 bytes per page write
    static constexpr size_t EEPROM_NUM_BLOCKS = 4;         // 4 blocks (0-3)
    static constexpr uint32_t EEPROM_WRITE_DELAY_MS = 5;   // Write cycle time (typical)

    class EEPROM
    {
    private:
        bool m_is_present;
        i2c_master_dev_handle_t m_i2c_device_handles[EEPROM_NUM_BLOCKS];
        SemaphoreHandle_t m_semaphore_handle;
        uint8_t m_base_address;

        EEPROM();
        ~EEPROM();

        // Internal helper methods
        esp_err_t read_internal(uint16_t address, uint8_t *data, size_t length);
        esp_err_t write_internal(uint16_t address, const uint8_t *data, size_t length);
        esp_err_t write_page_internal(uint16_t address, const uint8_t *data, size_t length);
        void wait_write_cycle();
        uint8_t get_block_number(uint16_t address);
        uint8_t get_block_address(uint16_t address);
        uint8_t get_device_address(uint16_t address);

    public:
        EEPROM(EEPROM const&) = delete;
        void operator=(EEPROM const &) = delete;
        static EEPROM &get_instance()
        {
            static EEPROM instance;
            return instance;
        }

        bool is_present() const { return m_is_present; }

        // Basic read/write operations
        esp_err_t read_byte(uint16_t address, uint8_t *data);
        esp_err_t write_byte(uint16_t address, uint8_t data);

        esp_err_t read(uint16_t address, uint8_t *data, size_t length);
        esp_err_t write(uint16_t address, const uint8_t *data, size_t length);

        // Page-optimized write (up to 16 bytes per page for faster writing)
        esp_err_t write_page(uint16_t address, const uint8_t *data, size_t length);

        // Utility functions
        esp_err_t clear(uint16_t address, size_t length);           // Clear (write 0x00)
        esp_err_t clear_all();                                      // Clear entire EEPROM
        esp_err_t fill(uint16_t address, size_t length, uint8_t value);  // Fill with value

        // Block operations (256 bytes per block)
        esp_err_t read_block(uint8_t block_num, uint8_t *data);    // Read entire block
        esp_err_t write_block(uint8_t block_num, const uint8_t *data);  // Write entire block
        esp_err_t clear_block(uint8_t block_num);                  // Clear entire block

        // Verification
        esp_err_t verify(uint16_t address, const uint8_t *data, size_t length);  // Verify written data

        // Utility information
        size_t get_total_size() const { return EEPROM_TOTAL_SIZE; }
        size_t get_block_size() const { return EEPROM_BLOCK_SIZE; }
        size_t get_page_size() const { return EEPROM_PAGE_SIZE; }
        size_t get_num_blocks() const { return EEPROM_NUM_BLOCKS; }
    };
}
