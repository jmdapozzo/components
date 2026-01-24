# PCF8574 I/O Expander Integration Guide

This guide explains how to integrate the I/O Expander component into your FlexCore Base application.

## Component Structure

The I/O Expander component is located in `managed_components/jmdapozzo__ioExpander/` with:
```
managed_components/jmdapozzo__ioExpander/
├── include/
│   └── ioExpander.hpp       # Header with IOExpander class
├── src/
│   └── ioExpander.cpp       # PCF8574 driver implementation
├── Kconfig                  # Configuration options
├── CMakeLists.txt           # Build configuration
├── idf_component.yml        # Component manifest
├── README.md                # Component documentation
└── INTEGRATION_GUIDE.md     # This file
```

## What's Been Created

### 1. Full PCF8574 Driver
- 8-bit I/O expander with I2C interface
- Individual pin control (read/write/toggle)
- Port-wide operations (all 8 pins at once)
- Multi-pin operations with bitmasks

### 2. Hardware Interrupt Support
- Interrupt on any pin change (INT pin → GPIO 5)
- Software debouncing (configurable)
- Per-pin interrupt modes (rising/falling/both)
- ISR handler with timer-based debounce

### 3. Callback System
- Per-pin callbacks using C++11 lambdas or function pointers
- Event loop integration for system-wide notifications
- Pin change events with old/new state
- Interrupt events with changed pin bitmask

### 4. Thread-Safe Design
- Semaphore-protected I2C operations
- Safe to call from multiple tasks
- ISR-safe interrupt handling

## Hardware Setup

### Required Connections

```
PCF8574          ESP32-S3
--------         --------
VDD        ---   3.3V
VSS        ---   GND
SCL        ---   GPIO 9 (your I2C SCL)
SDA        ---   GPIO 8 (your I2C SDA)
INT        ---   GPIO 5 (interrupt, optional)
A0         ---   GND/VDD (address bit 0)
A1         ---   GND/VDD (address bit 1)
A2         ---   GND/VDD (address bit 2)
P0-P7      ---   Your devices (LEDs, buttons, etc.)
```

### I2C Address Selection
Set I2C address using A0-A2 pins:
- All GND = 0x20 (default)
- A0=VDD, A1=GND, A2=GND = 0x21
- A0=GND, A1=VDD, A2=GND = 0x22
- etc.

### Pull-up Resistors
- **I2C Bus**: 4.7kΩ on SDA and SCL (usually on dev board)
- **INT Pin**: 10kΩ to 3.3V (optional, if using interrupts)
- **Input Pins**: External 10kΩ pulldown for buttons (or use internal weak pullup)

## Configuration Options

Available in `idf.py menuconfig` under "MacDap I/O Expander Configuration":

| Option | Default | Description |
|--------|---------|-------------|
| I2C Address | 0x20 | PCF8574 I2C address (0x20-0x27) |
| I2C Port | 0 | I2C port number |
| Enable Interrupt | Yes | Enable hardware interrupt support |
| Interrupt GPIO | 5 | GPIO connected to INT pin |
| Debounce Time | 50ms | Software debounce delay |

## Integration Steps

### Step 1: Add to Dependencies

Add to `main/idf_component.yml`:
```yaml
jmdapozzo/ioExpander:
    override_path: "../managed_components/jmdapozzo__ioExpander"
```

### Step 2: Include Header

Add to your main.cpp includes:
```cpp
#include <ioExpander.hpp>
```

### Step 3: Initialize in Main

Add after I2C initialization:
```cpp
// Get I/O Expander instance
macdap::IOExpander &io_expander = macdap::IOExpander::get_instance();

if (io_expander.is_present()) {
    ESP_LOGI(TAG, "PCF8574 I/O Expander detected");

    // Set event loop handle (if using events)
    io_expander.set_event_loop_handle(event_loop_handle);

    // Configure your pins here
    // Example: Pin 0 as output (LED), Pin 1 as input (button)
    io_expander.set_pin_mode(macdap::Pin0, macdap::PinOutput);
    io_expander.set_pin_mode(macdap::Pin1, macdap::PinInput);

    // Set up button interrupt
    io_expander.set_pin_interrupt(macdap::Pin1, macdap::InterruptFalling);
    io_expander.set_pin_callback(macdap::Pin1, [&io_expander](uint8_t pin, macdap::pin_state_t state) {
        ESP_LOGI(TAG, "Button pressed!");
        // Toggle LED on button press
        io_expander.toggle_pin(macdap::Pin0);
    });

    // Enable interrupts
    io_expander.enable_interrupt(true);
}
else {
    ESP_LOGW(TAG, "PCF8574 I/O Expander not found");
}
```

### Step 4: Use in Your Application

```cpp
// Turn LED on
io_expander.digital_write(macdap::Pin0, macdap::PinHigh);

// Read button state
macdap::pin_state_t button_state;
if (io_expander.digital_read(macdap::Pin1, &button_state) == ESP_OK) {
    if (button_state == macdap::PinLow) {
        ESP_LOGI(TAG, "Button is pressed");
    }
}

// Control multiple pins at once
io_expander.write_port(0b00001111);  // Set pins 0-3 HIGH, 4-7 LOW
```

## Common Integration Patterns

### Pattern 1: Button with LED Feedback

```cpp
// Pin 0 = LED (output), Pin 1 = Button (input)
io_expander.set_pin_mode(macdap::Pin0, macdap::PinOutput);
io_expander.set_pin_mode(macdap::Pin1, macdap::PinInput);

io_expander.set_pin_interrupt(macdap::Pin1, macdap::InterruptBoth);
io_expander.set_pin_callback(macdap::Pin1, [&](uint8_t pin, macdap::pin_state_t state) {
    // LED follows button (active low)
    io_expander.digital_write(macdap::Pin0, state == macdap::PinLow ? macdap::PinHigh : macdap::PinLow);
});
io_expander.enable_interrupt(true);
```

### Pattern 2: Status LEDs Array

```cpp
// Pins 0-3 as outputs for status LEDs
io_expander.set_pins_mode(0x0F, macdap::PinOutput);

// Show status with LED patterns
void show_status(uint8_t status) {
    io_expander.write_port(status & 0x0F);
}
```

### Pattern 3: Keypad Matrix (4 buttons)

```cpp
// Pins 0-3 as inputs for buttons
io_expander.set_pins_mode(0x0F, macdap::PinInput);

// Register handler for each button
for (int i = 0; i < 4; i++) {
    io_expander.set_pin_interrupt((macdap::io_pin_t)i, macdap::InterruptFalling);
    io_expander.set_pin_callback((macdap::io_pin_t)i, [i](uint8_t pin, macdap::pin_state_t state) {
        if (state == macdap::PinLow) {
            handle_button_press(i);
        }
    });
}
io_expander.enable_interrupt(true);
```

### Pattern 4: Event-Based Control

```cpp
void io_event_handler(void* arg, esp_event_base_t event_base,
                      int32_t event_id, void* event_data)
{
    if (event_id == macdap::IOExpanderPinChange) {
        auto* event = (macdap::io_expander_pin_event_t*)event_data;
        ESP_LOGI(TAG, "Pin %d: %d -> %d", event->pin, event->old_state, event->new_state);
    }
}

io_expander.register_event_handler(io_event_handler);
```

## Build and Test

### 1. Build
```bash
cd /Users/jmdapozzo/Projects/FlexCore/Applications/flexcore.base
idf.py build
```

### 2. Flash and Monitor
```bash
idf.py flash monitor
```

### 3. Expected Output
```
I (xxxx) io_expander: Initializing PCF8574 I/O Expander...
I (xxxx) io_expander: Found PCF8574 at address 0x20
I (xxxx) io_expander: Interrupt configured on GPIO 5
I (xxxx) io_expander: Initialization complete
I (xxxx) main: PCF8574 I/O Expander detected
```

## Testing

### Test 1: Basic Output (LED Blink)
```cpp
while (true) {
    io_expander.digital_write(macdap::Pin0, macdap::PinHigh);
    vTaskDelay(pdMS_TO_TICKS(500));
    io_expander.digital_write(macdap::Pin0, macdap::PinLow);
    vTaskDelay(pdMS_TO_TICKS(500));
}
```

### Test 2: Input Reading (Button)
```cpp
macdap::pin_state_t state;
io_expander.digital_read(macdap::Pin1, &state);
ESP_LOGI(TAG, "Button: %s", state == macdap::PinHigh ? "Released" : "Pressed");
```

### Test 3: Interrupt Response
- Press a button connected to an input pin
- Check logs for callback execution
- Verify debouncing works (no multiple triggers)

## Troubleshooting

### Device Not Found
1. Check I2C wiring (SDA, SCL, VDD, GND)
2. Verify I2C address matches A0-A2 configuration
3. Check for I2C pullup resistors
4. Run `i2cdetect` to scan bus

### Interrupts Not Working
1. Verify INT pin connected to GPIO 5
2. Check INT has pullup resistor (10kΩ)
3. Ensure `enable_interrupt(true)` is called
4. Verify pin interrupt mode is set
5. Check debounce time isn't too long

### Input Pins Always HIGH
1. PCF8574 has weak internal pullups (~100μA)
2. Add external pulldown (10kΩ) for buttons
3. Or use stronger external pullup (4.7kΩ)
4. Buttons should connect pin to GND when pressed

### Outputs Don't Work
1. PCF8574 can only sink current (active LOW)
2. For LEDs: Cathode to pin, anode to VDD via resistor
3. Cannot source significant current on HIGH
4. Use external pullup or transistor for HIGH drive

## Moving to GitHub

When ready to publish:

1. Copy component to your repository:
```bash
cp -r managed_components/jmdapozzo__ioExpander /path/to/components/ioExpander
```

2. Update `main/idf_component.yml`:
```yaml
jmdapozzo/ioExpander:
    version: main
    git: https://github.com/jmdapozzo/components.git
    path: ioExpander
```

3. Commit and push to GitHub

4. Run `idf.py reconfigure`

## Additional Resources

- **Component API**: See [README.md](README.md)
- **PCF8574 Datasheet**: NXP PCF8574/PCF8574A datasheet
- **Example Code**: See README.md for usage examples

## Support

For issues or questions:
- Review [README.md](README.md) for API details
- Check [ioExpander.hpp](include/ioExpander.hpp) for class definition
- Review [ioExpander.cpp](src/ioExpander.cpp) for implementation details
