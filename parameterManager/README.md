# Parameter Manager Component

A comprehensive parameter management component for ESP-IDF with NVS storage and change notification callbacks.

## Features

- **NVS-backed persistent storage** - Parameters survive reboots
- **Thread-safe access** - Safe for multi-threaded applications
- **Type-safe operations** - Supports String, Integer, Float, and Boolean types
- **Change notifications** - Register callbacks for parameter changes
- **Default values** - Optional default values for parameters
- **Easy API** - Simple create, get, set, delete operations
- **Parameter listing** - Enumerate all registered parameters

## API Overview

### Creating Parameters

```cpp
#include <parameterManager.hpp>

macdap::ParameterManager& params = macdap::ParameterManager::get_instance();

// Create string parameter with default
const char* default_tz = "UTC";
params.create_parameter("timezone",
                       macdap::ParameterManager::ParameterType::String,
                       default_tz);

// Create integer parameter
int32_t default_port = 8080;
params.create_parameter("server_port",
                       macdap::ParameterManager::ParameterType::Integer,
                       &default_port);
```

### Getting and Setting Values

```cpp
// Get string parameter
char timezone[64];
params.get_string("timezone", timezone, sizeof(timezone));

// Set string parameter (triggers callback if registered)
params.set_string("timezone", "America/New_York");

// Get/Set integer
int32_t port;
params.get_int("server_port", &port);
params.set_int("server_port", 9000);
```

### Callbacks for Changes

Multiple consumers can register their own callbacks for the same parameter. Each callback will be triggered when the parameter value changes.

```cpp
void timezone_changed_callback(const char* id, void* user_data) {
    ESP_LOGI("APP", "Timezone parameter changed: %s", id);
    // Update timezone in time manager
}

void display_timezone_callback(const char* id, void* user_data) {
    ESP_LOGI("DISPLAY", "Updating timezone display: %s", id);
    // Update timezone on display
}

// Multiple callbacks can be registered for the same parameter
params.register_callback("timezone", timezone_changed_callback, nullptr);
params.register_callback("timezone", display_timezone_callback, nullptr);

// Unregister specific callback
params.unregister_callback("timezone", timezone_changed_callback);

// Or unregister all callbacks for a parameter
params.unregister_callback("timezone", nullptr);
```

### Other Operations

```cpp
// Check if parameter exists
if (params.exists("timezone")) {
    // ...
}

// Reset to default value
params.reset_to_default("timezone");

// Delete parameter
params.delete_parameter("old_param");

// List all parameters
char param_ids[50][33];
size_t count;
params.list_parameters(param_ids, 50, &count);
```

## Supported Parameter Types

- `ParameterType::String` - Null-terminated strings
- `ParameterType::Integer` - 32-bit signed integers
- `ParameterType::Float` - 32-bit floating point
- `ParameterType::Boolean` - Boolean (true/false)

## Configuration

Configure via menuconfig:
```
Component config → Parameter Manager
```

### Configuration Options

**Maximum number of parameters** (`CONFIG_PARAMETER_MANAGER_MAX_PARAMS`)
- Default: 50
- Range: 1-200
- Limits the total number of parameters that can be registered
- When exceeded, `create_parameter()` returns `ESP_ERR_NO_MEM`
- Each parameter consumes memory for metadata storage

**NVS namespace for parameters** (`CONFIG_PARAMETER_MANAGER_NVS_NAMESPACE`)
- Default: "sys_params"
- Maximum length: 15 characters
- NVS namespace used for storing all parameters
- Must be unique across your application

**Enable detailed parameter logging** (`CONFIG_PARAMETER_MANAGER_ENABLE_LOGGING`)
- Default: yes
- When enabled: Logs detailed information about parameter operations (create, delete, callbacks)
- When disabled: Only logs errors and warnings
- Useful for debugging but adds to code size and log output

## Thread Safety

All operations are protected by a mutex, making the component safe for concurrent access from multiple tasks. Key thread-safety features:

- **Mutex Protection**: All parameter operations (get/set/create/delete) are protected by a FreeRTOS mutex
- **Multiple Callbacks**: Each consumer can register its own callback without interfering with others
- **Callback Isolation**: Callbacks are called outside of the mutex lock to prevent deadlocks
- **Safe from ISR**: While the component itself is thread-safe for task context, callbacks should not be registered from ISR context

## Integration Example

```cpp
#include <parameterManager.hpp>

extern "C" void app_main(void)
{
    // Get singleton instance
    macdap::ParameterManager& params = macdap::ParameterManager::get_instance();

    // Create application parameters
    const char* default_ssid = "MyNetwork";
    params.create_parameter("wifi_ssid",
                           macdap::ParameterManager::ParameterType::String,
                           default_ssid);

    bool default_enabled = true;
    params.create_parameter("feature_enabled",
                           macdap::ParameterManager::ParameterType::Boolean,
                           &default_enabled);

    // Register callback for changes
    params.register_callback("feature_enabled", feature_toggle_callback, nullptr);

    // Use parameters
    char ssid[32];
    if (params.get_string("wifi_ssid", ssid, sizeof(ssid)) == ESP_OK) {
        ESP_LOGI("APP", "WiFi SSID: %s", ssid);
    }
}
```

## Future Enhancements

This component is designed to support future integration with:
- Web-based configuration interface
- Bluetooth parameter editing
- Remote parameter management

When parameters are changed via web/Bluetooth, registered callbacks will be triggered automatically, allowing your application to respond to configuration changes in real-time.

## License

MIT License - See LICENSE file for details
