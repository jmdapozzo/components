#pragma once

#include "esp_err.h"
#include "esp_event.h"
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include <functional>

ESP_EVENT_DECLARE_BASE(IO_EXPANDER_EVENTS);

namespace macdap
{
    typedef enum {
        IOExpanderPinChange,        // A pin state changed
        IOExpanderInterrupt         // Interrupt triggered
    } io_expander_event_id_t;

    typedef enum {
        Pin0 = 0,
        Pin1 = 1,
        Pin2 = 2,
        Pin3 = 3,
        Pin4 = 4,
        Pin5 = 5,
        Pin6 = 6,
        Pin7 = 7
    } io_pin_t;

    typedef enum {
        PinLow = 0,
        PinHigh = 1
    } pin_state_t;

    typedef enum {
        PinOutput = 0,      // Pin configured as output (driven low or high)
        PinInput = 1        // Pin configured as input (high impedance with weak pullup)
    } pin_mode_t;

    typedef enum {
        InterruptNone = 0,      // No interrupt
        InterruptRising = 1,    // Trigger on rising edge
        InterruptFalling = 2,   // Trigger on falling edge
        InterruptBoth = 3       // Trigger on any edge
    } pin_interrupt_mode_t;

    typedef struct {
        uint8_t pin;                    // Pin number (0-7)
        pin_state_t old_state;          // Previous state
        pin_state_t new_state;          // Current state
        uint8_t port_value;             // Full 8-bit port value
    } io_expander_pin_event_t;

    typedef struct {
        uint8_t port_value;             // Full 8-bit port value
        uint8_t changed_pins;           // Bitmask of changed pins
    } io_expander_interrupt_event_t;

    // Callback function type for pin change notifications
    typedef std::function<void(uint8_t pin, pin_state_t state)> pin_change_callback_t;

    class IOExpander
    {
    private:
        bool m_is_present;
        i2c_master_dev_handle_t m_i2c_device_handle;
        SemaphoreHandle_t m_semaphore_handle;
        esp_event_loop_handle_t m_event_loop_handle;

        uint8_t m_last_port_value;      // Last read port value for change detection
        uint8_t m_pin_modes;            // Current pin modes (1=input, 0=output)
        uint8_t m_output_values;        // Last written output values

        gpio_num_t m_interrupt_gpio;
        bool m_interrupt_enabled;
        esp_timer_handle_t m_debounce_timer;

        pin_change_callback_t m_pin_callbacks[8];  // Per-pin callbacks
        pin_interrupt_mode_t m_pin_interrupt_modes[8];  // Per-pin interrupt modes

        IOExpander();
        ~IOExpander();

        // Internal helper methods
        esp_err_t read_port_internal(uint8_t *value);
        esp_err_t write_port_internal(uint8_t value);
        void handle_interrupt();
        static void IRAM_ATTR gpio_isr_handler(void* arg);
        static void debounce_timer_callback(void* arg);

    public:
        IOExpander(IOExpander const&) = delete;
        void operator=(IOExpander const &) = delete;
        static IOExpander &get_instance()
        {
            static IOExpander instance;
            return instance;
        }

        bool is_present() const { return m_is_present; }
        esp_event_loop_handle_t get_event_loop_handle() const { return m_event_loop_handle; }
        void set_event_loop_handle(esp_event_loop_handle_t event_loop_handle) { m_event_loop_handle = event_loop_handle; }

        // Port operations (all 8 pins at once)
        esp_err_t read_port(uint8_t *value);           // Read all 8 pins
        esp_err_t write_port(uint8_t value);           // Write all 8 pins

        // Individual pin operations
        esp_err_t set_pin_mode(io_pin_t pin, pin_mode_t mode);
        esp_err_t get_pin_mode(io_pin_t pin, pin_mode_t *mode);
        esp_err_t digital_write(io_pin_t pin, pin_state_t state);
        esp_err_t digital_read(io_pin_t pin, pin_state_t *state);
        esp_err_t toggle_pin(io_pin_t pin);

        // Multi-pin operations
        esp_err_t set_pins_mode(uint8_t pin_mask, pin_mode_t mode);  // Set multiple pins at once
        esp_err_t digital_write_mask(uint8_t pin_mask, pin_state_t state);  // Write multiple pins

        // Interrupt configuration
        esp_err_t enable_interrupt(bool enable);
        esp_err_t set_pin_interrupt(io_pin_t pin, pin_interrupt_mode_t mode);
        esp_err_t get_pin_interrupt(io_pin_t pin, pin_interrupt_mode_t *mode);

        // Callback registration (per-pin callbacks)
        esp_err_t set_pin_callback(io_pin_t pin, pin_change_callback_t callback);
        esp_err_t clear_pin_callback(io_pin_t pin);

        // Event handling
        esp_err_t register_event_handler(esp_event_handler_t event_handler, void* handler_arg = nullptr);
        esp_err_t unregister_event_handler(esp_event_handler_t event_handler);

        // Utility functions
        esp_err_t poll();  // Manual poll for pin changes (useful if not using interrupts)
    };
}
