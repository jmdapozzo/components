# MacDap I/O Expander Component

ESP-IDF component for the PCF8574/PCF8574A 8-Bit I2C I/O Expander with interrupt support.

## Features

- **8 I/O Pins**: Quasi-bidirectional I/O pins (P0-P7)
- **Individual Pin Control**: Read/write individual pins or all 8 at once
- **Interrupt Support**: Hardware interrupt on any pin change (INT pin)
- **Debouncing**: Software debouncing for mechanical switches
- **Per-Pin Callbacks**: Register callbacks for individual pin changes
- **Event System**: Integrated with ESP-IDF event loop
- **Thread Safe**: Semaphore-protected I2C operations
- **Flexible Interrupt Modes**: Rising edge, falling edge, or both

## Hardware Requirements

- **IC**: PCF8574 or PCF8574A I/O Expander
- **I2C**: Connected to ESP32 I2C bus
- **Interrupt** (optional): INT pin connected to ESP32 GPIO

## Wiring

### Basic Wiring (No Interrupt)

| PCF8574 Pin | ESP32 Pin | Description |
|-------------|-----------|-------------|
| VDD | 3.3V | Power supply |
| VSS | GND | Ground |
| SCL | GPIO 9 (default) | I2C Clock |
| SDA | GPIO 8 (default) | I2C Data |
| A0-A2 | GND/VDD | Address select pins |

### With Interrupt Support

| PCF8574 Pin | ESP32 Pin | Description |
|-------------|-----------|-------------|
| VDD | 3.3V | Power supply |
| VSS | GND | Ground |
| SCL | GPIO 9 (default) | I2C Clock |
| SDA | GPIO 8 (default) | I2C Data |
| INT | GPIO 5 (default) | Interrupt output (active low) |
| A0-A2 | GND/VDD | Address select pins |

### I2C Addresses

**PCF8574**: 0x20-0x27 (A2-A1-A0 in binary added to 0x20)
- A2=0, A1=0, A0=0 → 0x20 (default)
- A2=0, A1=0, A0=1 → 0x21
- A2=0, A1=1, A0=0 → 0x22
- ... and so on

**PCF8574A**: 0x38-0x3F (same pattern, different base address)

## Configuration

Configure using `idf.py menuconfig`:

```
Component config → MacDap I/O Expander Configuration
```

Available options:
- **I/O Expander Model**: PCF8574/PCF8574A
- **I2C Address**: 0x20 (default, adjust based on A0-A2 pins)
- **I2C Port Number**: 0 (default)
- **Enable Interrupt**: Yes (recommended)
- **Interrupt GPIO**: 5 (default)
- **Debounce Time**: 50ms (default)

## Usage

### Basic Digital I/O

```cpp
#include "ioExpander.hpp"

using namespace macdap;

// Get the singleton instance
IOExpander& io = IOExpander::get_instance();

if (io.is_present()) {
    // Set pin 0 as output
    io.set_pin_mode(Pin0, PinOutput);

    // Write HIGH to pin 0
    io.digital_write(Pin0, PinHigh);

    // Set pin 1 as input
    io.set_pin_mode(Pin1, PinInput);

    // Read pin 1
    pin_state_t state;
    if (io.digital_read(Pin1, &state) == ESP_OK) {
        ESP_LOGI("APP", "Pin 1 is %s", state == PinHigh ? "HIGH" : "LOW");
    }

    // Toggle pin 0
    io.toggle_pin(Pin0);
}
```

### Multiple Pins at Once

```cpp
// Set pins 0-3 as outputs, 4-7 as inputs
io.set_pins_mode(0x0F, PinOutput);  // Pins 0-3
io.set_pins_mode(0xF0, PinInput);   // Pins 4-7

// Write HIGH to pins 0 and 2, LOW to pins 1 and 3
io.digital_write_mask(0x05, PinHigh);  // 0b00000101 = pins 0 and 2
io.digital_write_mask(0x0A, PinLow);   // 0b00001010 = pins 1 and 3

// Read all 8 pins at once
uint8_t port_value;
io.read_port(&port_value);
ESP_LOGI("APP", "All pins: 0x%02X", port_value);
```

### Using Callbacks (Simple)

```cpp
// Define a callback function
void button_pressed(uint8_t pin, pin_state_t state) {
    ESP_LOGI("APP", "Button on pin %d is now %s",
             pin, state == PinHigh ? "released" : "pressed");
}

// Set pin 2 as input
io.set_pin_mode(Pin2, PinInput);

// Register callback for falling edge (button press)
io.set_pin_interrupt(Pin2, InterruptFalling);
io.set_pin_callback(Pin2, button_pressed);

// Enable interrupts
io.enable_interrupt(true);
```

### Using Lambda Callbacks

```cpp
// Set up LED on pin 0 that follows button on pin 1
io.set_pin_mode(Pin0, PinOutput);
io.set_pin_mode(Pin1, PinInput);

io.set_pin_interrupt(Pin1, InterruptBoth);
io.set_pin_callback(Pin1, [&io](uint8_t pin, pin_state_t state) {
    // When button is pressed (LOW), turn LED on
    io.digital_write(Pin0, state == PinLow ? PinHigh : PinLow);
});

io.enable_interrupt(true);
```

### Using Event System

```cpp
// Event handler function
void io_event_handler(void* arg, esp_event_base_t event_base,
                      int32_t event_id, void* event_data)
{
    if (event_id == IOExpanderPinChange) {
        io_expander_pin_event_t* event = (io_expander_pin_event_t*)event_data;
        ESP_LOGI("APP", "Pin %d changed from %d to %d",
                 event->pin, event->old_state, event->new_state);
    }
    else if (event_id == IOExpanderInterrupt) {
        io_expander_interrupt_event_t* event = (io_expander_interrupt_event_t*)event_data;
        ESP_LOGI("APP", "Interrupt! Port=0x%02X, Changed=0x%02X",
                 event->port_value, event->changed_pins);
    }
}

// In your initialization code
IOExpander& io = IOExpander::get_instance();
if (io.is_present()) {
    // Set event loop (typically from your main event loop)
    io.set_event_loop_handle(event_loop_handle);

    // Register event handler
    io.register_event_handler(io_event_handler);

    // Configure pins and interrupts
    io.set_pin_mode(Pin1, PinInput);
    io.set_pin_interrupt(Pin1, InterruptBoth);
    io.enable_interrupt(true);
}
```

### Without Hardware Interrupt

If you don't connect the INT pin or want to poll manually:

```cpp
// Configure in menuconfig: disable interrupt or set GPIO to -1

// In your main loop or timer callback
void poll_timer_callback(void* arg) {
    IOExpander& io = IOExpander::get_instance();
    io.poll();  // Manually check for pin changes
}

// Create a periodic timer to poll
esp_timer_create_args_t timer_args = {
    .callback = poll_timer_callback,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "IOPoll"
};
esp_timer_handle_t timer;
esp_timer_create(&timer_args, &timer);
esp_timer_start_periodic(timer, 100000);  // Poll every 100ms
```

## Common Use Cases

### 1. Button Matrix (4 buttons)

```cpp
// Pins 0-3 as inputs with pullups (buttons to GND)
io.set_pins_mode(0x0F, PinInput);

// Set up interrupts for all button pins
for (int i = 0; i < 4; i++) {
    io.set_pin_interrupt((io_pin_t)i, InterruptFalling);
    io.set_pin_callback((io_pin_t)i, [i](uint8_t pin, pin_state_t state) {
        if (state == PinLow) {
            ESP_LOGI("APP", "Button %d pressed", i);
        }
    });
}

io.enable_interrupt(true);
```

### 2. LED Array (8 LEDs)

```cpp
// All pins as outputs
io.set_pins_mode(0xFF, PinOutput);

// Turn on LEDs 0, 2, 4, 6 (even LEDs)
io.write_port(0b01010101);

// Blink all LEDs
while (true) {
    io.write_port(0xFF);
    vTaskDelay(pdMS_TO_TICKS(500));
    io.write_port(0x00);
    vTaskDelay(pdMS_TO_TICKS(500));
}
```

### 3. Relay Control (4 relays + 4 status inputs)

```cpp
// Pins 0-3 as outputs (relays), 4-7 as inputs (status)
io.set_pins_mode(0x0F, PinOutput);
io.set_pins_mode(0xF0, PinInput);

// Turn on relay 0
io.digital_write(Pin0, PinHigh);

// Check status input 4
pin_state_t status;
io.digital_read(Pin4, &status);
```

### 4. Rotary Encoder

```cpp
// Pins 0 and 1 for encoder A and B
io.set_pin_mode(Pin0, PinInput);
io.set_pin_mode(Pin1, PinInput);

int encoder_position = 0;
pin_state_t last_a = PinHigh;

io.set_pin_interrupt(Pin0, InterruptBoth);
io.set_pin_callback(Pin0, [&](uint8_t pin, pin_state_t a_state) {
    pin_state_t b_state;
    io.digital_read(Pin1, &b_state);

    if (last_a == PinHigh && a_state == PinLow) {
        encoder_position += (b_state == PinHigh) ? 1 : -1;
        ESP_LOGI("APP", "Encoder: %d", encoder_position);
    }
    last_a = a_state;
});

io.enable_interrupt(true);
```

## PCF8574 Operation Notes

### Quasi-Bidirectional I/O
The PCF8574 has quasi-bidirectional pins:
- **Writing 1**: Pin becomes input (high impedance with 100μA pullup)
- **Writing 0**: Pin drives LOW (output mode, ~25mA sink capability)
- **Reading**: Always reads actual pin state regardless of configuration

This means:
1. To use as **input**: Write 1 (enables weak pullup), then read
2. To use as **output**: Write 0 or 1 directly
3. External pullups (4.7kΩ) recommended for reliable input operation

### Power-On State
All pins default to HIGH (input mode) after power-on.

### Current Limits
- **Output LOW**: Can sink up to 25mA per pin, 100mA total
- **Output HIGH**: Cannot source significant current (use external pullup)
- **VDD**: Maximum 26mA total through VDD pin

### Interrupt (INT) Pin
- Active LOW (goes low on any input change)
- Open-drain output (needs pullup resistor)
- Cleared automatically by reading the port
- Change detected from last read value, not last written value

## API Reference

### Pin Control
- `set_pin_mode(pin, mode)` - Configure pin as input or output
- `digital_write(pin, state)` - Write HIGH or LOW to pin
- `digital_read(pin, *state)` - Read current pin state
- `toggle_pin(pin)` - Toggle output pin state

### Port Control
- `read_port(*value)` - Read all 8 pins (returns 8-bit value)
- `write_port(value)` - Write all 8 pins (8-bit value)

### Multi-Pin Control
- `set_pins_mode(mask, mode)` - Set mode for multiple pins
- `digital_write_mask(mask, state)` - Write to multiple pins

### Interrupts
- `enable_interrupt(enable)` - Enable/disable hardware interrupt
- `set_pin_interrupt(pin, mode)` - Set interrupt mode per pin
- `set_pin_callback(pin, callback)` - Register callback for pin
- `clear_pin_callback(pin)` - Remove pin callback
- `poll()` - Manually poll for changes (if not using interrupts)

### Event System
- `set_event_loop_handle(handle)` - Set ESP event loop handle
- `register_event_handler(handler, arg)` - Register event handler
- `unregister_event_handler(handler)` - Unregister event handler

### Utility
- `is_present()` - Check if I/O expander detected
- `get_event_loop_handle()` - Get current event loop handle

## Troubleshooting

### I/O Expander Not Detected
- Check wiring (VDD, GND, SCL, SDA)
- Verify I2C address with A0-A2 pins
- Check for I2C pullup resistors (4.7kΩ typical)
- Use `i2cdetect` to scan bus

### Inputs Always Read HIGH
- PCF8574 has weak pullups (~100μA)
- Add external 10kΩ pulldown for button inputs
- Or use external 4.7kΩ pullup for better noise immunity

### Interrupts Not Working
- Verify INT pin connection to GPIO 5
- Check INT has pullup resistor (10kΩ typical)
- Ensure interrupt enabled with `enable_interrupt(true)`
- Set interrupt mode with `set_pin_interrupt()`
- INT clears when port is read

### Outputs Won't Drive HIGH
- PCF8574 cannot source current (weak pullup only)
- Use external pullup resistor (1-10kΩ) for HIGH output
- Or use active pull-up transistor circuit
- For LEDs: Connect cathode to pin, anode to VDD (active LOW)

## License

MIT License - See component LICENSE file for details.

## Author

MacDap Components - https://github.com/jmdapozzo/components
