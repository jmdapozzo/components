# 24LC08B EEPROM Integration Guide

Quick guide to integrate the EEPROM component into your FlexCore Base application.

## Component Structure

```
managed_components/jmdapozzo__eeprom/
├── include/
│   └── eeprom.hpp           # Header with EEPROM class
├── src/
│   └── eeprom.cpp           # 24LC08B driver implementation
├── Kconfig                  # Configuration options
├── CMakeLists.txt           # Build configuration
├── idf_component.yml        # Component manifest
├── README.md                # Component documentation
└── INTEGRATION_GUIDE.md     # This file
```

## What's Been Created

### 1. Complete 24LC08B Driver
- 1024 bytes (1KB) of non-volatile storage
- 4 blocks of 256 bytes each
- Fast page writes (16 bytes per page)
- Automatic block addressing
- Thread-safe operations

### 2. Features
- Single byte and multi-byte read/write
- Page-optimized writes for speed
- Block operations (256 bytes)
- Clear, fill, and verify utilities
- Persistent storage across ESP32 flash erases

## Hardware Setup

### Wiring

```
24LC08B          ESP32-S3
-------          --------
VCC (8)    ---   3.3V
VSS (4)    ---   GND
SCL (6)    ---   GPIO 9 (I2C SCL)
SDA (5)    ---   GPIO 8 (I2C SDA)
WP (7)     ---   GND (write protection disabled)
A0 (1)     ---   GND (must be GND)
A1 (2)     ---   GND (must be GND)
A2 (3)     ---   GND (must be GND)
```

**Important**:
- A0, A1, A2 must all be GND for 24LC08B
- WP should be GND for normal operation (VCC enables write protection)
- I2C pullup resistors (4.7kΩ) on SDA and SCL

### I2C Addresses
The 24LC08B uses 4 I2C addresses for its 4 blocks:
- Block 0 (addresses 0x000-0x0FF): I2C 0x50
- Block 1 (addresses 0x100-0x1FF): I2C 0x51
- Block 2 (addresses 0x200-0x2FF): I2C 0x52
- Block 3 (addresses 0x300-0x3FF): I2C 0x53

The driver handles this automatically - you use linear addressing (0-1023).

## Configuration

Available in `idf.py menuconfig` under "MacDap EEPROM Configuration":

| Option | Default | Description |
|--------|---------|-------------|
| EEPROM Model | 24LC08B | EEPROM type |
| I2C Base Address | 0x50 | Base I2C address |
| I2C Port | 0 | I2C port number |

## Why Use EEPROM vs NVS?

| Feature | EEPROM | ESP32 NVS |
|---------|--------|-----------|
| **Survives flash erase** | ✅ Yes | ❌ No |
| **Capacity** | 1024 bytes | Varies (partition-based) |
| **Write cycles** | 1M per byte | 100K per sector |
| **Speed** | ~5ms per write | Faster |
| **Use case** | Permanent data (serial #, cal data) | Runtime config |

**Use EEPROM for**:
- Device serial numbers
- Hardware calibration data
- Manufacturing data
- Permanent identifiers
- Data that must survive ESP32 reprogramming

**Use NVS for**:
- User configuration
- WiFi credentials
- Runtime settings
- Frequently changing data

## Common Usage Patterns

### 1. Device Serial Number

```cpp
#define EEPROM_SERIAL_ADDR 0x000

// Write serial number (one time, during manufacturing)
void write_serial_number(const char* serial) {
    eeprom.write(EEPROM_SERIAL_ADDR, (uint8_t*)serial, 16);
}

// Read serial number
void read_serial_number(char* serial) {
    eeprom.read(EEPROM_SERIAL_ADDR, (uint8_t*)serial, 16);
}
```

### 2. Calibration Data

```cpp
#define EEPROM_CAL_ADDR 0x100

struct CalibrationData {
    uint32_t magic;         // 0xCAFEBABE
    float temperature_offset;
    float voltage_gain;
    uint16_t crc16;
};

// Save calibration
void save_calibration(CalibrationData& cal) {
    cal.magic = 0xCAFEBABE;
    // Calculate CRC16...
    eeprom.write(EEPROM_CAL_ADDR, (uint8_t*)&cal, sizeof(cal));
}

// Load calibration
bool load_calibration(CalibrationData& cal) {
    eeprom.read(EEPROM_CAL_ADDR, (uint8_t*)&cal, sizeof(cal));
    if (cal.magic != 0xCAFEBABE) {
        return false;  // Not calibrated
    }
    // Verify CRC16...
    return true;
}
```

### 3. Manufacturing Data

```cpp
#define EEPROM_MFG_ADDR 0x200

struct ManufacturingData {
    uint32_t magic;
    uint8_t hw_revision;
    uint16_t manufacture_date;  // Days since epoch
    char product_code[16];
    char test_result[32];
};

void write_manufacturing_data(ManufacturingData& mfg) {
    mfg.magic = 0xDEADBEEF;
    eeprom.write(EEPROM_MFG_ADDR, (uint8_t*)&mfg, sizeof(mfg));
}
```

### 4. Persistent Counter (Wear Leveling)

```cpp
// Spread writes across 256 locations to extend life
#define COUNTER_BLOCK 3
#define COUNTER_START (COUNTER_BLOCK * 256)
#define COUNTER_ENTRIES 64

uint32_t read_counter_with_wear_leveling() {
    uint32_t max_value = 0;
    uint16_t max_index = 0;

    // Find highest counter value
    for (int i = 0; i < COUNTER_ENTRIES; i++) {
        uint32_t value;
        eeprom.read(COUNTER_START + (i * 4), (uint8_t*)&value, 4);
        if (value > max_value && value != 0xFFFFFFFF) {
            max_value = value;
            max_index = i;
        }
    }

    return max_value;
}

void increment_counter_with_wear_leveling() {
    uint32_t current = read_counter_with_wear_leveling();
    uint32_t next = current + 1;

    // Write to next location
    uint16_t write_index = (current % COUNTER_ENTRIES);
    eeprom.write(COUNTER_START + (write_index * 4), (uint8_t*)&next, 4);
}
```

## Memory Layout Planning

Plan your EEPROM layout for your application:

```
Address Range    Size     Purpose
0x000 - 0x00F    16 B     Device Serial Number
0x010 - 0x02F    32 B     Device Identifier
0x030 - 0x0FF    208 B    Reserved

0x100 - 0x11F    32 B     Calibration Data
0x120 - 0x1FF    224 B    Reserved

0x200 - 0x2FF    256 B    Manufacturing Data / Test Results

0x300 - 0x3FF    256 B    Persistent Logs / Counters
```

Create a header file for your layout:

```cpp
// eeprom_layout.h
#define EEPROM_SERIAL_ADDR       0x000
#define EEPROM_SERIAL_SIZE       16

#define EEPROM_DEVICE_ID_ADDR    0x010
#define EEPROM_DEVICE_ID_SIZE    32

#define EEPROM_CAL_ADDR          0x100
#define EEPROM_CAL_SIZE          32

#define EEPROM_MFG_ADDR          0x200
#define EEPROM_MFG_SIZE          256

#define EEPROM_LOG_ADDR          0x300
#define EEPROM_LOG_SIZE          256
```

## Build and Test

### Build
```bash
idf.py build
idf.py flash monitor
```

### Expected Output
```
I (xxxx) eeprom: Initializing 24LC08B EEPROM...
I (xxxx) eeprom: Found 24LC08B at base address 0x50
I (xxxx) eeprom: Initialization complete: 1024 bytes (4 blocks x 256 bytes)
I (xxxx) main: 24LC08B EEPROM detected
```

### Testing

```cpp
// Simple test in your main app
void test_eeprom() {
    EEPROM& eeprom = EEPROM::get_instance();

    if (!eeprom.is_present()) {
        ESP_LOGE(TAG, "EEPROM not available!");
        return;
    }

    // Test write and read
    const char* test_str = "Hello EEPROM!";
    eeprom.write(0x00, (uint8_t*)test_str, strlen(test_str) + 1);

    char read_buf[32];
    eeprom.read(0x00, (uint8_t*)read_buf, sizeof(read_buf));

    ESP_LOGI(TAG, "EEPROM test: %s", read_buf);

    // Verify
    if (eeprom.verify(0x00, (uint8_t*)test_str, strlen(test_str) + 1) == ESP_OK) {
        ESP_LOGI(TAG, "EEPROM verification OK");
    }
}
```

## Troubleshooting

### EEPROM Not Detected
1. Check I2C wiring (SDA, SCL, VCC, GND)
2. Verify A0, A1, A2 are all connected to GND
3. Check for I2C pullup resistors (4.7kΩ)
4. Verify power supply (2.5V-5.5V)

### Writes Don't Work
1. Check WP pin is connected to GND (not VCC)
2. Ensure addresses are within range (0-1023)
3. Wait for write cycle completion (~5ms)

### Data Corruption
1. Use `verify()` after critical writes
2. Add CRC/checksum to stored data
3. Ensure stable power during writes
4. Don't write during power-down

## Moving to GitHub

When ready to publish:

1. Copy component:
```bash
cp -r managed_components/jmdapozzo__eeprom /path/to/components/eeprom
```

2. Update `main/idf_component.yml`:
```yaml
jmdapozzo/eeprom:
    version: main
    git: https://github.com/jmdapozzo/components.git
    path: eeprom
```

3. Reconfigure:
```bash
idf.py reconfigure
idf.py build
```

## Best Practices

1. **Plan Your Layout**: Define memory map before using
2. **Use Structures**: Store data in versioned structures
3. **Add Checksums**: Always verify critical data
4. **Minimize Writes**: 1M cycle limit per byte
5. **Wear Leveling**: Spread frequent writes across addresses
6. **Magic Numbers**: Use magic values to detect uninitialized EEPROM
7. **Version Data**: Include version numbers for future compatibility

## Example: Complete Device Info

```cpp
#include "eeprom.hpp"

#define EEPROM_DEVICE_INFO_ADDR 0x000

struct DeviceInfo {
    uint32_t magic;              // 0x12345678
    uint8_t version;             // Structure version
    char serial_number[16];      // e.g., "SN123456789"
    uint8_t hw_revision;         // Hardware revision
    uint16_t manufacture_year;   // Year
    uint8_t manufacture_month;   // Month (1-12)
    uint8_t manufacture_day;     // Day (1-31)
    char product_code[16];       // e.g., "FLEXCORE-BASE"
    uint16_t crc16;              // Data integrity check
};

// Write device info (one time, during manufacturing)
void write_device_info(const DeviceInfo& info) {
    EEPROM& eeprom = EEPROM::get_instance();

    DeviceInfo data = info;
    data.magic = 0x12345678;
    data.version = 1;
    // Calculate CRC16 here...

    eeprom.write(EEPROM_DEVICE_INFO_ADDR, (uint8_t*)&data, sizeof(data));

    // Verify
    if (eeprom.verify(EEPROM_DEVICE_INFO_ADDR, (uint8_t*)&data, sizeof(data)) == ESP_OK) {
        ESP_LOGI(TAG, "Device info written successfully");
    }
}

// Read device info
bool read_device_info(DeviceInfo& info) {
    EEPROM& eeprom = EEPROM::get_instance();

    eeprom.read(EEPROM_DEVICE_INFO_ADDR, (uint8_t*)&info, sizeof(info));

    // Verify magic number
    if (info.magic != 0x12345678) {
        ESP_LOGE(TAG, "Invalid device info (not programmed)");
        return false;
    }

    // Verify CRC16...

    ESP_LOGI(TAG, "Device: %s, S/N: %s, Rev: %d",
             info.product_code, info.serial_number, info.hw_revision);

    return true;
}
```

## Reference

- Component Documentation: [README.md](README.md)
- Main Configuration: See integration code in main.cpp
- 24LC08B Datasheet: Microchip 24LC08B
