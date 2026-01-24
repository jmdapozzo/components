# MacDap EEPROM Component

ESP-IDF component for the 24LC08B I2C EEPROM (1KB non-volatile storage).

## Features

- **1024 Bytes**: 8Kbit (1KB) of non-volatile storage
- **I2C Interface**: 400kHz I2C communication
- **Page Writes**: Fast 16-byte page writes
- **Block Organization**: 4 blocks of 256 bytes each
- **Read/Write Operations**: Single byte or multi-byte
- **Utility Functions**: Clear, fill, verify
- **Thread Safe**: Semaphore-protected operations
- **Long Life**: 1,000,000 write cycles per byte, 200-year data retention

## Hardware Requirements

- **IC**: 24LC08B EEPROM (Microchip)
- **Capacity**: 8Kbit (1024 bytes)
- **I2C**: Connected to ESP32 I2C bus
- **Organization**: 4 x 256 byte blocks

## Wiring

| 24LC08B Pin | ESP32 Pin | Description |
|-------------|-----------|-------------|
| VCC (8) | 3.3V | Power supply (2.5V-5.5V) |
| VSS (4) | GND | Ground |
| SCL (6) | GPIO 9 (default) | I2C Clock |
| SDA (5) | GPIO 8 (default) | I2C Data |
| WP (7) | GND | Write Protect (GND=disabled, VCC=enabled) |
| A0 (1) | GND | Address bit 0 (must be GND) |
| A1 (2) | GND | Address bit 1 (must be GND) |
| A2 (3) | GND | Address bit 2 (must be GND) |

**Important**: A0, A1, A2 must all be connected to GND for 24LC08B.

## Memory Organization

The 24LC08B has 1024 bytes organized as:
- **4 Blocks**: 256 bytes each (blocks 0-3)
- **I2C Addresses**: 0x50, 0x51, 0x52, 0x53 (one per block)
- **Page Size**: 16 bytes per page
- **Total Pages**: 64 pages (16 per block)

```
Address Range    Block    I2C Address
0x000 - 0x0FF    Block 0    0x50
0x100 - 0x1FF    Block 1    0x51
0x200 - 0x2FF    Block 2    0x52
0x300 - 0x3FF    Block 3    0x53
```

## Configuration

Configure using `idf.py menuconfig`:

```
Component config → MacDap EEPROM Configuration
```

Available options:
- **EEPROM Model**: 24LC08B (1KB)
- **I2C Base Address**: 0x50 (default)
- **I2C Port**: 0 (default)

## Usage

### Basic Read/Write

```cpp
#include "eeprom.hpp"

using namespace macdap;

// Get the singleton instance
EEPROM& eeprom = EEPROM::get_instance();

if (eeprom.is_present()) {
    // Write a single byte
    eeprom.write_byte(0x00, 0xAA);

    // Read a single byte
    uint8_t value;
    eeprom.read_byte(0x00, &value);
    ESP_LOGI("APP", "Read: 0x%02X", value);

    // Write multiple bytes
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    eeprom.write(0x10, data, sizeof(data));

    // Read multiple bytes
    uint8_t buffer[4];
    eeprom.read(0x10, buffer, sizeof(buffer));
}
```

### Fast Page Writes

```cpp
// Write up to 16 bytes at once (page write)
uint8_t page_data[16] = {0};
for (int i = 0; i < 16; i++) {
    page_data[i] = i;
}

// Write a full page (must be page-aligned)
eeprom.write_page(0x00, page_data, 16);  // Address 0x00 is page-aligned

// Or use write() which handles page alignment automatically
eeprom.write(0x05, page_data, 16);  // Handles alignment for you
```

### Clear and Fill

```cpp
// Clear (write zeros) to a range
eeprom.clear(0x00, 256);  // Clear first 256 bytes

// Clear entire EEPROM
eeprom.clear_all();  // Clears all 1024 bytes

// Fill with a specific value
eeprom.fill(0x00, 100, 0xFF);  // Fill first 100 bytes with 0xFF
```

### Block Operations

```cpp
// Read an entire block (256 bytes)
uint8_t block_data[256];
eeprom.read_block(0, block_data);  // Read block 0

// Write an entire block
uint8_t write_data[256];
memset(write_data, 0xAA, 256);
eeprom.write_block(1, write_data);  // Write to block 1

// Clear a block
eeprom.clear_block(2);  // Clear block 2
```

### Data Verification

```cpp
// Write and verify
uint8_t data[] = "Hello EEPROM!";
eeprom.write(0x00, data, sizeof(data));

// Verify written data
if (eeprom.verify(0x00, data, sizeof(data)) == ESP_OK) {
    ESP_LOGI("APP", "Data verified successfully");
} else {
    ESP_LOGE("APP", "Verification failed!");
}
```

### Storing Structures

```cpp
// Define a structure
struct Config {
    uint32_t magic;
    uint8_t version;
    char name[16];
    uint16_t checksum;
};

Config config = {
    .magic = 0xDEADBEEF,
    .version = 1,
    .name = "MyDevice",
    .checksum = 0x1234
};

// Write structure to EEPROM
eeprom.write(0x00, (uint8_t*)&config, sizeof(Config));

// Read structure from EEPROM
Config read_config;
eeprom.read(0x00, (uint8_t*)&read_config, sizeof(Config));

// Verify
if (read_config.magic == 0xDEADBEEF) {
    ESP_LOGI("APP", "Valid config: %s, version %d", read_config.name, read_config.version);
}
```

### Persistent Counter

```cpp
// Read counter from EEPROM
uint32_t boot_count = 0;
eeprom.read(0x00, (uint8_t*)&boot_count, sizeof(boot_count));

// Increment
boot_count++;

// Write back to EEPROM
eeprom.write(0x00, (uint8_t*)&boot_count, sizeof(boot_count));

ESP_LOGI("APP", "Boot count: %d", boot_count);
```

### String Storage

```cpp
// Write string
const char* message = "Hello from EEPROM!";
eeprom.write(0x00, (uint8_t*)message, strlen(message) + 1);  // +1 for null terminator

// Read string
char buffer[32];
eeprom.read(0x00, (uint8_t*)buffer, sizeof(buffer));
ESP_LOGI("APP", "Message: %s", buffer);
```

### Calibration Data Storage

```cpp
struct CalibrationData {
    float offset;
    float gain;
    uint16_t temperature;
    uint16_t crc;
};

// Store calibration
CalibrationData cal = {1.05f, 2.3f, 2500, 0};
// Calculate CRC here...
eeprom.write(0x100, (uint8_t*)&cal, sizeof(cal));

// Load calibration
CalibrationData loaded_cal;
eeprom.read(0x100, (uint8_t*)&loaded_cal, sizeof(loaded_cal));
// Verify CRC...
```

## Common Use Cases

### 1. Configuration Storage

```cpp
#define EEPROM_CONFIG_ADDR 0x000

void save_config(const char* ssid, const char* password) {
    uint8_t config_data[64] = {0};

    // Pack configuration
    snprintf((char*)config_data, 32, "%s", ssid);
    snprintf((char*)&config_data[32], 32, "%s", password);

    eeprom.write(EEPROM_CONFIG_ADDR, config_data, sizeof(config_data));
}

void load_config(char* ssid, char* password) {
    uint8_t config_data[64];
    eeprom.read(EEPROM_CONFIG_ADDR, config_data, sizeof(config_data));

    strncpy(ssid, (char*)config_data, 32);
    strncpy(password, (char*)&config_data[32], 32);
}
```

### 2. Device ID Storage

```cpp
#define EEPROM_DEVICE_ID_ADDR 0x3F0  // Last 16 bytes

void write_device_id(const uint8_t* device_id, size_t len) {
    eeprom.write(EEPROM_DEVICE_ID_ADDR, device_id, len);
}

void read_device_id(uint8_t* device_id, size_t len) {
    eeprom.read(EEPROM_DEVICE_ID_ADDR, device_id, len);
}
```

### 3. Logging Events

```cpp
#define EEPROM_LOG_START 0x200
#define EEPROM_LOG_SIZE 256
#define LOG_ENTRY_SIZE 16

void log_event(uint8_t event_type, uint32_t timestamp) {
    static uint8_t log_index = 0;

    uint8_t entry[LOG_ENTRY_SIZE] = {0};
    entry[0] = event_type;
    memcpy(&entry[1], &timestamp, sizeof(timestamp));

    uint16_t addr = EEPROM_LOG_START + (log_index * LOG_ENTRY_SIZE);
    eeprom.write(addr, entry, LOG_ENTRY_SIZE);

    log_index = (log_index + 1) % (EEPROM_LOG_SIZE / LOG_ENTRY_SIZE);
}
```

## Performance Characteristics

### Write Speed
- **Single Byte**: ~5ms (includes write cycle time)
- **Page Write** (16 bytes): ~5ms (much faster per byte)
- **Full EEPROM** (1024 bytes): ~320ms using page writes

### Read Speed
- **Single Byte**: <1ms
- **Sequential Read**: ~400kHz I2C speed (up to 50KB/s theoretical)
- **Full EEPROM**: ~20-30ms

### Endurance
- **Write Cycles**: 1,000,000 cycles per byte
- **Data Retention**: 200 years (typical)
- **Temperature**: -40°C to +85°C

## API Reference

### Basic Operations
- `read_byte(address, *data)` - Read single byte
- `write_byte(address, data)` - Write single byte
- `read(address, *data, length)` - Read multiple bytes
- `write(address, *data, length)` - Write multiple bytes (page-optimized)
- `write_page(address, *data, length)` - Explicit page write (max 16 bytes)

### Utility Functions
- `clear(address, length)` - Clear (write 0x00)
- `clear_all()` - Clear entire EEPROM
- `fill(address, length, value)` - Fill with value

### Block Operations
- `read_block(block_num, *data)` - Read 256-byte block
- `write_block(block_num, *data)` - Write 256-byte block
- `clear_block(block_num)` - Clear 256-byte block

### Verification
- `verify(address, *data, length)` - Verify written data

### Information
- `is_present()` - Check if EEPROM detected
- `get_total_size()` - Returns 1024
- `get_block_size()` - Returns 256
- `get_page_size()` - Returns 16
- `get_num_blocks()` - Returns 4

## Important Notes

### Write Cycle Time
- After each write, the EEPROM requires ~5ms to complete the internal write cycle
- During this time, the EEPROM will not respond to I2C
- The driver automatically waits for the write cycle to complete

### Page Write Boundaries
- Page writes can write up to 16 bytes in one operation
- **Cannot cross page boundaries** - address must be page-aligned
- Use `write()` function which handles page alignment automatically
- Use `write_page()` only if you're sure about page alignment

### Write Protection
- WP pin controls write protection
- WP = GND: Writes enabled (normal operation)
- WP = VCC: All writes blocked (read-only mode)

### Address Space
- Valid addresses: 0x000 to 0x3FF (0 to 1023)
- The driver handles block addressing automatically
- You can use linear addressing (0-1023) without worrying about blocks

## Thread Safety

All operations are thread-safe. The component uses a semaphore to protect I2C transactions, so you can safely access the EEPROM from multiple tasks.

```cpp
// Safe to use from multiple tasks
xTaskCreate(task1, "task1", 2048, NULL, 5, NULL);
xTaskCreate(task2, "task2", 2048, NULL, 5, NULL);
```

## Troubleshooting

### EEPROM Not Detected
- Check I2C wiring (SDA, SCL, VCC, GND)
- Verify base address is 0x50
- Check A0, A1, A2 pins are all connected to GND
- Verify I2C pullup resistors (4.7kΩ)

### Write Failures
- Check WP pin is connected to GND (not VCC)
- Ensure address is within range (0-1023)
- Check power supply is stable (2.5V-5.5V)

### Data Corruption
- Verify data with `verify()` function after writing
- Ensure power is stable during writes
- Don't interrupt writes (wait for write cycle)

### Slow Writes
- Use page writes instead of byte-by-byte
- Use `write()` function which optimizes page usage
- Remember: each write takes ~5ms regardless of size (up to 16 bytes)

## Best Practices

1. **Use Page Writes**: Always use `write()` instead of multiple `write_byte()` calls
2. **Verify Critical Data**: Use `verify()` after writing important configuration
3. **Minimize Writes**: EEPROM has limited write cycles (1M), read is unlimited
4. **Use Checksums**: Add CRC/checksum to stored data to detect corruption
5. **Version Your Data**: Include version numbers in stored structures
6. **Reserve Space**: Plan your memory layout to allow for future expansion

## License

MIT License - See component LICENSE file for details.

## Author

MacDap Components - https://github.com/jmdapozzo/components
