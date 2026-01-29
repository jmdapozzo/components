# Status Buzzer Basic Example

This example demonstrates how to use the StatusBuzzer component to drive a SMT-0540 magnetic transducer.

## Features Demonstrated

- Initialization of the buzzer on a GPIO pin
- Different sound patterns (slow/medium/fast chirps and beeps)
- Continuous tone
- Silent mode

## Hardware Setup

Connect the SMT-0540 magnetic transducer:
- Positive terminal to the configured GPIO pin (default: GPIO 5)
- Negative terminal to GND

## Configuration

Before building, configure the buzzer settings using `idf.py menuconfig`:
- Navigate to "MacDap Status Buzzer"
- Set the GPIO pin number
- Set the frequency (default 4000Hz for SMT-0540)

## Building and Running

```bash
idf.py build
idf.py flash monitor
```

The example cycles through all available sound patterns with a 4000Hz square wave.
