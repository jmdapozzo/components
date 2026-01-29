# BLE Parameter Service

Bluetooth Low Energy GATT service for parameter management via mobile apps.

## Overview

This component provides a complete BLE GATT service that mirrors the REST API functionality for parameter management but is optimized for mobile app integration. It allows mobile apps to manage device parameters over BLE without requiring WiFi connectivity.

## Features

- **Parameter List**: Read list of all parameters with their types
- **Parameter Access**: Get and set individual parameter values
- **Parameter Delete**: Remove parameters
- **Real-time Notifications**: Receive parameter change notifications
- **Control Commands**: Execute system commands (restart, factory reset, etc.)

## Service UUID

- Service: `12340000-1234-1234-1234-123456789abc`

## Characteristics

| Characteristic | UUID | Properties | Description |
|---------------|------|------------|-------------|
| Parameter List | `12340001-...` | Read, Notify | List all parameters (JSON) |
| Parameter Access | `12340002-...` | Read, Write | Get/Set parameter (JSON) |
| Parameter Delete | `12340003-...` | Write | Delete parameter by name (JSON) |
| Parameter Notify | `12340004-...` | Notify | Parameter change notifications (JSON) |
| Control | `12340005-...` | Write | System control commands (JSON) |

## Usage

### Initialization

```cpp
#include "bleParameterService.hpp"

// Get singleton instance
macdap::BleParameterService& bleService = macdap::BleParameterService::get_instance();

// Initialize BLE stack
esp_err_t err = bleService.init();
if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize BLE service");
}

// Start advertising
err = bleService.start();
if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start BLE service");
}

// Enable parameter change notifications
bleService.enable_notifications(true);
```

### JSON Format Examples

#### Parameter List (Read Response)
```json
{
  "count": 3,
  "parameters": [
    {
      "name": "wifi_ssid",
      "type": "string"
    },
    {
      "name": "temperature_threshold",
      "type": "float"
    },
    {
      "name": "auto_restart",
      "type": "boolean"
    }
  ]
}
```

#### Parameter Access (Write - Set Value)
```json
{
  "name": "temperature_threshold",
  "value": 30.0
}
```

#### Parameter Access (Read Response)
```json
{
  "name": "wifi_ssid",
  "type": "string",
  "value": "MyNetwork",
  "status": "ok"
}
```

#### Parameter Delete
```json
{
  "name": "old_parameter"
}
```

#### Control Commands
```json
{
  "command": "restart"
}
```

Available commands:
- `refresh_list` - Trigger parameter list notification
- `restart` - Restart the device
- `factory_reset` - Erase all settings and restart
- `reset_to_defaults` - Reset all parameters to default values

#### Parameter Change Notification
```json
{
  "event": "parameter_changed",
  "name": "wifi_ssid",
  "type": "string",
  "value": "NewNetwork",
  "timestamp": 1234567890
}
```

## Mobile App Integration

### iOS (Swift) Example

```swift
import CoreBluetooth

let paramServiceUUID = CBUUID(string: "12340000-1234-1234-1234-123456789abc")
let accessCharUUID = CBUUID(string: "12340002-1234-1234-1234-123456789abc")

// Set parameter
func setParameter(name: String, value: Any) {
    let json = ["name": name, "value": value]
    let data = try? JSONSerialization.data(withJSONObject: json)
    peripheral?.writeValue(data, for: accessCharacteristic, type: .withResponse)
}
```

### Android (Kotlin) Example

```kotlin
import android.bluetooth.BluetoothGatt
import org.json.JSONObject

val PARAM_SERVICE_UUID = UUID.fromString("12340000-1234-1234-1234-123456789abc")
val ACCESS_CHAR_UUID = UUID.fromString("12340002-1234-1234-1234-123456789abc")

fun setParameter(name: String, value: Any) {
    val json = JSONObject().apply {
        put("name", name)
        put("value", value)
    }
    val characteristic = gatt.getService(PARAM_SERVICE_UUID)
        .getCharacteristic(ACCESS_CHAR_UUID)
    characteristic.value = json.toString().toByteArray()
    gatt.writeCharacteristic(characteristic)
}
```

## Testing

Use nRF Connect mobile app (iOS/Android) to test the service:

1. Scan for device name "FlexCore Params"
2. Connect to the device
3. Discover services and find the Parameter Management Service
4. Read Parameter List characteristic to see all parameters
5. Write to Parameter Access characteristic to set values
6. Enable notifications to receive real-time updates

## Dependencies

- ESP-IDF BLE/Bluetooth stack
- ParameterManager component
- cJSON for JSON encoding/decoding
- NVS Flash for persistent storage

## Configuration

No specific Kconfig options required. The service automatically:
- Negotiates MTU up to 512 bytes
- Handles parameter type validation
- Provides thread-safe access via ParameterManager
- Persists all changes to NVS

## Notes

- Single connection supported (one mobile app at a time)
- All characteristics use JSON for data interchange
- Parameter changes via BLE trigger the same callbacks as REST API
- Control commands (restart/factory_reset) execute after 1-second delay

## License

Copyright (c) 2026 MacDap
