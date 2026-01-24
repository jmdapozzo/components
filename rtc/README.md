# MacDap RTC Component

ESP-IDF component for the MCP7940 Real-Time Clock (RTC) with battery backup support.

## Features

- **Time Management**: Get/set time with automatic BCD conversion
- **System Integration**: Sync between RTC and system time
- **Battery Backup**: Maintain time during power loss
- **Power Failure Detection**: Track power-down and power-up timestamps
- **Alarm Support**: Two independent alarms with multiple match modes
- **Square Wave Output**: Configurable frequency output on MFP pin
- **SRAM**: 64 bytes of battery-backed SRAM for user data
- **Thread Safe**: Semaphore-protected I2C operations

## Hardware Requirements

- **IC**: MCP7940M or MCP7940N
- **Battery**: CR2032 or similar 3V battery (optional, for backup)
- **Crystal**: 32.768 kHz (usually included in RTC modules)
- **I2C**: Connected to ESP32 I2C bus

## Wiring

| MCP7940 Pin | ESP32 Pin | Description |
|-------------|-----------|-------------|
| VCC | 3.3V | Power supply |
| GND | GND | Ground |
| SCL | GPIO 9 (default) | I2C Clock |
| SDA | GPIO 8 (default) | I2C Data |
| VBAT | CR2032+ | Battery positive (optional) |
| MFP | GPIO (optional) | Multi-Function Pin (alarm/square wave) |

## Configuration

Configure the component using `idf.py menuconfig` under "MacDap RTC Configuration":

- **I2C RTC Address**: Default 0x6F
- **I2C Port Number**: Default 0
- **Auto-sync to system on boot**: Sync system time from RTC at startup
- **Auto-sync from NTP**: Update RTC when NTP sync completes
- **Enable battery backup**: Enable battery backup by default

## Usage

### Basic Time Operations

```cpp
#include "rtc.hpp"

using namespace macdap;

// Get the singleton instance
RTC& rtc = RTC::get_instance();

// Check if RTC is present
if (rtc.is_present()) {
    // Get time from RTC
    struct tm time_info;
    if (rtc.get_time(&time_info) == ESP_OK) {
        ESP_LOGI("APP", "RTC Time: %04d-%02d-%02d %02d:%02d:%02d",
                 time_info.tm_year + 1900, time_info.tm_mon + 1, time_info.tm_mday,
                 time_info.tm_hour, time_info.tm_min, time_info.tm_sec);
    }

    // Set time manually
    struct tm new_time = {
        .tm_sec = 0,
        .tm_min = 30,
        .tm_hour = 14,
        .tm_mday = 23,
        .tm_mon = 0,  // January (0-11)
        .tm_year = 126,  // 2026 (years since 1900)
        .tm_wday = 4  // Thursday (0-6)
    };
    rtc.set_time(&new_time);

    // Start the oscillator
    rtc.start();
}
```

### System Time Synchronization

```cpp
// Sync system time from RTC (useful on boot)
rtc.sync_to_system();

// Sync RTC from system time (after NTP update)
rtc.sync_from_system();
```

### Battery Backup

```cpp
// Enable battery backup
rtc.enable_battery_backup(true);

// Check if power was lost
bool lost_power;
if (rtc.has_lost_power(&lost_power) == ESP_OK && lost_power) {
    ESP_LOGW("APP", "Power was lost!");

    // Get timestamps
    struct tm powerdown, powerup;
    rtc.get_power_down_timestamp(&powerdown);
    rtc.get_power_up_timestamp(&powerup);

    // Clear the flag
    rtc.clear_power_fail_flag();
}
```

### Alarms

```cpp
// Set alarm for 7:30 AM every day
struct tm alarm_time = {
    .tm_sec = 0,
    .tm_min = 30,
    .tm_hour = 7
};
rtc.set_alarm(Alarm1, &alarm_time, AlarmMatchHours);
rtc.enable_alarm(Alarm1, true);

// Check if alarm triggered
bool triggered;
if (rtc.is_alarm_triggered(Alarm1, &triggered) == ESP_OK && triggered) {
    ESP_LOGI("APP", "Alarm triggered!");
    rtc.clear_alarm_flag(Alarm1);
}
```

### SRAM Storage

```cpp
// Write data to SRAM (64 bytes available)
uint8_t data[4] = {0xDE, 0xAD, 0xBE, 0xEF};
rtc.write_sram(0, data, sizeof(data));

// Read data from SRAM
uint8_t buffer[4];
rtc.read_sram(0, buffer, sizeof(buffer));
```

### Square Wave Output

```cpp
// Enable 1Hz square wave on MFP pin
rtc.enable_square_wave(true);
rtc.set_square_wave_frequency(0);  // 0=1Hz, 1=4.096kHz, 2=8.192kHz, 3=32.768kHz
```

## Integration with NTP

Example integration in your main app:

```cpp
void ntp_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI("APP", "NTP time synchronized");

    // Update RTC from system time after NTP sync
    if (CONFIG_RTC_AUTO_SYNC_FROM_NTP) {
        RTC& rtc = RTC::get_instance();
        if (rtc.is_present()) {
            rtc.sync_from_system();
        }
    }
}

void app_main()
{
    // ... I2C initialization ...

    // Get RTC instance
    RTC& rtc = RTC::get_instance();

    if (rtc.is_present()) {
        // Enable battery backup
        if (CONFIG_RTC_ENABLE_BATTERY_BACKUP) {
            rtc.enable_battery_backup(true);
        }

        // Check if oscillator is running
        bool running;
        rtc.is_running(&running);
        if (!running) {
            ESP_LOGW("APP", "RTC oscillator stopped, starting...");
            rtc.start();
        }

        // Sync system time from RTC on boot
        if (CONFIG_RTC_AUTO_SYNC_TO_SYSTEM) {
            rtc.sync_to_system();
        }
    }

    // ... Continue with WiFi/NTP initialization ...
}
```

## API Reference

See [rtc.hpp](include/rtc.hpp) for complete API documentation.

## License

MIT License - See component LICENSE file for details.

## Author

MacDap Components - https://github.com/jmdapozzo/components
