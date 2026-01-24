# MCP7940 RTC Integration Guide

This guide explains how the RTC component has been integrated into your FlexCore Base application.

## What's Been Done

### 1. Component Structure Created
The RTC component is located in `managed_components/jmdapozzo__rtc/` with the following structure:
```
managed_components/jmdapozzo__rtc/
├── include/
│   └── rtc.hpp          # Header with RTC class definition
├── src/
│   └── rtc.cpp          # Implementation with MCP7940 driver
├── Kconfig              # Configuration options
├── CMakeLists.txt       # Build configuration
├── idf_component.yml    # Component manifest
├── README.md            # Component documentation
└── INTEGRATION_GUIDE.md # This file
```

### 2. Main Application Integration
The component has been integrated into your main application:

- **Header included** in [main.cpp:64](main/src/main.cpp#L64)
- **RTC initialization** in [main.cpp:1258-1318](main/src/main.cpp#L1258) (after temperature sensor, before NVS)
- **NTP sync callback updated** in [main.cpp:268-285](main/src/main.cpp#L268) to sync RTC when NTP updates
- **Dependency added** to [main/idf_component.yml](main/idf_component.yml)

### 3. Features Implemented
- Automatic system time sync from RTC on boot (if time is valid)
- Automatic RTC sync from system time after NTP sync
- Battery backup support
- Power failure detection with timestamps
- Thread-safe I2C operations
- Two independent alarms
- 64 bytes of battery-backed SRAM
- Square wave output capability

## Hardware Setup

### Required Components
- **MCP7940M or MCP7940N** RTC IC
- **32.768 kHz crystal** (usually included in RTC modules)
- **CR2032 battery** (optional, for battery backup)

### Wiring
Connect the MCP7940 to your ESP32-S3:

| MCP7940 Pin | ESP32 Pin | Description |
|-------------|-----------|-------------|
| VCC | 3.3V | Power supply |
| GND | GND | Ground |
| SCL | GPIO 9 | I2C Clock (your configured pin) |
| SDA | GPIO 8 | I2C Data (your configured pin) |
| VBAT | CR2032+ | Battery positive (optional) |
| X1/X2 | Crystal | 32.768 kHz crystal |
| MFP | - | Multi-function pin (optional) |

**Note:** GPIO 8 and 9 are already configured as your I2C bus in your main application.

## Configuration

Configure the RTC using `idf.py menuconfig`:

```
Component config → MacDap RTC Configuration
```

Available options:
- **RTC Model**: MCP7940 (default)
- **I2C RTC Address**: 0x6F (default MCP7940 address)
- **I2C Port Number**: 0 (default, matches your setup)
- **Auto sync to system on boot**: Enabled (recommended)
- **Auto sync from NTP**: Enabled (recommended)
- **Enable battery backup**: Enabled (recommended)

## Build and Test

### 1. Clean and Reconfigure
```bash
cd /Users/jmdapozzo/Projects/FlexCore/Applications/flexcore.base
idf.py fullclean
idf.py reconfigure
```

### 2. Configure (if needed)
```bash
idf.py menuconfig
```
Navigate to "MacDap RTC Configuration" and verify settings.

### 3. Build
```bash
idf.py build
```

### 4. Flash and Monitor
```bash
idf.py flash monitor
```

### 5. Expected Log Output
When the RTC is connected, you should see:
```
I (xxxx) rtc: Initializing MCP7940 RTC...
I (xxxx) rtc: Found MCP7940 at address 0x6F
I (xxxx) rtc: Battery backup: enabled
I (xxxx) rtc: Oscillator: running
I (xxxx) rtc: Initialization complete
I (xxxx) main: MCP7940 RTC detected
I (xxxx) main: System time synchronized from RTC: 2026-01-23 14:30:00
```

If RTC is not connected:
```
W (xxxx) rtc: MCP7940 not found at address 0x6F
W (xxxx) main: MCP7940 RTC not found, time will only be maintained via NTP
```

## Testing the RTC

### Test 1: Check RTC Time
After NTP sync, the RTC should be updated. Check the logs for:
```
I (xxxx) main: NTP time synchronized
I (xxxx) main: RTC synchronized from NTP time
```

### Test 2: Power Cycle Test
1. Let NTP sync and update the RTC
2. Disconnect from power (ensure battery is installed)
3. Wait a few minutes
4. Reconnect power and monitor
5. You should see: `I (xxxx) main: System time synchronized from RTC: 2026-01-23 14:35:23`

### Test 3: Power Failure Detection
If power was lost and restored, you'll see:
```
W (xxxx) main: RTC detected power failure
I (xxxx) main: Power down: 01/23 14:30
I (xxxx) main: Power up: 01/23 14:35
```

## Advanced Usage

### Reading RTC Time Manually
```cpp
#include <rtc.hpp>

macdap::RTC& rtc = macdap::RTC::get_instance();
if (rtc.is_present()) {
    struct tm time_info;
    if (rtc.get_time(&time_info) == ESP_OK) {
        ESP_LOGI("APP", "RTC: %04d-%02d-%02d %02d:%02d:%02d",
                 time_info.tm_year + 1900, time_info.tm_mon + 1, time_info.tm_mday,
                 time_info.tm_hour, time_info.tm_min, time_info.tm_sec);
    }
}
```

### Setting an Alarm
```cpp
// Set alarm for 7:30 AM every day
struct tm alarm_time = {
    .tm_sec = 0,
    .tm_min = 30,
    .tm_hour = 7
};
rtc.set_alarm(macdap::Alarm1, &alarm_time, macdap::AlarmMatchHours);
rtc.enable_alarm(macdap::Alarm1, true);
```

### Using SRAM for Persistent Data
```cpp
// Write data that survives power cycles (with battery)
uint8_t boot_count = 0;
rtc.read_sram(0, &boot_count, 1);
boot_count++;
rtc.write_sram(0, &boot_count, 1);
ESP_LOGI("APP", "Boot count: %d", boot_count);
```

## Troubleshooting

### RTC Not Detected
- **Check wiring**: Verify SDA and SCL connections
- **Check I2C address**: Use `i2c-tools` or logic analyzer to verify 0x6F
- **Check power**: Ensure 3.3V on VCC pin
- **Check pull-ups**: I2C needs pull-up resistors (usually on dev boards)

### Time Not Persistent
- **Check battery**: Ensure CR2032 is installed and has voltage
- **Check VBATEN**: Battery backup must be enabled in config
- **Check crystal**: 32.768 kHz crystal must be connected

### Build Errors
If you get compilation errors:
```bash
idf.py fullclean
rm -rf managed_components/jmdapozzo__rtc/.component_hash
idf.py reconfigure
idf.py build
```

## Moving to GitHub Repository

When you're ready to publish this component to your GitHub repository:

1. Copy the component to your components repo:
```bash
cp -r managed_components/jmdapozzo__rtc /path/to/components/rtc
```

2. Update [main/idf_component.yml](main/idf_component.yml):
```yaml
# Remove the override_path line and uncomment:
jmdapozzo/rtc:
    version: main
    git: https://github.com/jmdapozzo/components.git
    path: rtc
```

3. Commit and push to your components repository

4. Run `idf.py reconfigure` to fetch from GitHub

## API Reference

For complete API documentation, see [README.md](README.md).

## Support

For issues or questions:
- Check the [README.md](README.md) for API details
- Review your main.cpp integration at [main/src/main.cpp](main/src/main.cpp)
- Check the component source at [src/rtc.cpp](src/rtc.cpp)
