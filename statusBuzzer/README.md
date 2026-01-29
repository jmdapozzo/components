# Status Buzzer Component

A status indication component for ESP-IDF that drives a SMT-0540 magnetic transducer to provide audible feedback.

## Features

- Multiple audible patterns (chirps, beeps, continuous tone)
- Fixed 4000Hz square wave (50% duty cycle) optimized for SMT-0540
- Singleton pattern for easy access throughout the application
- Efficient esp_timer-based pattern generation
- Low memory footprint

## Supported Patterns

- **Silent**: No sound
- **Slow Chirp**: Short burst (50ms) every 2 seconds
- **Medium Chirp**: Short burst (50ms) every 1 second
- **Fast Chirp**: Short burst (50ms) every 0.5 seconds
- **Slow Beep**: Longer tone (200ms) every 2 seconds
- **Medium Beep**: Longer tone (200ms) every 1 second
- **Fast Beep**: Longer tone (200ms) every 0.5 seconds
- **Continuous**: Constant tone

## Usage Example

```cpp
#include "statusBuzzer.hpp"

// Get the singleton instance
auto& buzzer = macdap::StatusBuzzer::get_instance();

// Initialize the buzzer on GPIO 5
buzzer.init(GPIO_NUM_5);

// Set status
buzzer.set_status(macdap::BuzzerStatus::FastBeep);

// Stop the buzzer
buzzer.stop();
```

## Configuration

The component can be configured through menuconfig:

- `STATUS_BUZZER_GPIO`: GPIO pin number (default: 5)
- `STATUS_BUZZER_FREQUENCY`: Operating frequency in Hz (default: 4000Hz)

## Hardware

This component is designed for the SMT-0540 magnetic transducer, which requires a square wave at 4000Hz for optimal operation. The buzzer should be connected between the configured GPIO pin and ground.

**Note**: Volume control is not supported as magnetic transducers require a pure square wave (50% duty cycle) and cannot be driven with PWM duty cycle variations.

## License

MIT License
