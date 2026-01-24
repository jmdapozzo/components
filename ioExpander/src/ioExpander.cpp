#include "ioExpander.hpp"
#include <esp_log.h>
#include <esp_err.h>
#include <esp_check.h>
#include <string.h>

using namespace macdap;

// PCF8574 I2C Address (can be 0x20-0x27 depending on A0-A2 pins)
#define PCF8574_I2CADDR_DEFAULT     0x20

static const char *TAG = "io_expander";

ESP_EVENT_DEFINE_BASE(IO_EXPANDER_EVENTS);

IOExpander::IOExpander()
{
    ESP_LOGI(TAG, "Initializing PCF8574 I/O Expander...");

    m_is_present = false;
    m_event_loop_handle = nullptr;
    m_last_port_value = 0xFF;
    m_pin_modes = 0xFF;  // All pins default to input
    m_output_values = 0xFF;
    m_interrupt_gpio = (gpio_num_t)CONFIG_IO_EXPANDER_INTERRUPT_GPIO;
    m_interrupt_enabled = false;
    m_debounce_timer = nullptr;

    // Initialize pin callbacks and interrupt modes
    for (int i = 0; i < 8; i++) {
        m_pin_callbacks[i] = nullptr;
        m_pin_interrupt_modes[i] = InterruptNone;
    }

    // Get I2C master bus handle
    i2c_master_bus_handle_t i2c_master_bus_handle;
    ESP_ERROR_CHECK(i2c_master_get_bus_handle(CONFIG_I2C_PORT_NUM, &i2c_master_bus_handle));

    // Probe for the I/O Expander device
    if (i2c_master_probe(i2c_master_bus_handle, CONFIG_I2C_IO_EXPANDER_ADDR, 100) != ESP_OK)
    {
        ESP_LOGW(TAG, "PCF8574 not found at address 0x%02X", CONFIG_I2C_IO_EXPANDER_ADDR);
        return;
    }

    ESP_LOGI(TAG, "Found PCF8574 at address 0x%02X", CONFIG_I2C_IO_EXPANDER_ADDR);
    m_is_present = true;

    // Create semaphore for thread safety
    m_semaphore_handle = xSemaphoreCreateMutex();
    if (m_semaphore_handle == nullptr)
    {
        ESP_LOGE(TAG, "Failed to create semaphore");
        m_is_present = false;
        return;
    }

    xSemaphoreTake(m_semaphore_handle, 0);

    // Configure I2C device
    i2c_device_config_t i2c_device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CONFIG_I2C_IO_EXPANDER_ADDR,
        .scl_speed_hz = 100000,  // 100 kHz
        .scl_wait_us = 0,
        .flags = {}
    };

    if (i2c_master_bus_add_device(i2c_master_bus_handle, &i2c_device_config, &m_i2c_device_handle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add I2C device");
        xSemaphoreGive(m_semaphore_handle);
        m_is_present = false;
        return;
    }

    // Initialize all pins as inputs (write 0xFF)
    if (write_port_internal(0xFF) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize I/O expander");
        xSemaphoreGive(m_semaphore_handle);
        m_is_present = false;
        return;
    }

    // Read initial port value
    if (read_port_internal(&m_last_port_value) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read initial port value");
        xSemaphoreGive(m_semaphore_handle);
        m_is_present = false;
        return;
    }

    xSemaphoreGive(m_semaphore_handle);

    // Configure interrupt GPIO if enabled
    #ifdef CONFIG_IO_EXPANDER_ENABLE_INTERRUPT
    if (CONFIG_IO_EXPANDER_INTERRUPT_GPIO >= 0)
    {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << m_interrupt_gpio),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_NEGEDGE  // PCF8574 INT is active low
        };

        if (gpio_config(&io_conf) != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to configure interrupt GPIO");
        }
        else
        {
            // Install GPIO ISR service if not already installed
            gpio_install_isr_service(0);

            // Add ISR handler
            if (gpio_isr_handler_add(m_interrupt_gpio, gpio_isr_handler, this) != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to add ISR handler");
            }
            else
            {
                ESP_LOGI(TAG, "Interrupt configured on GPIO %d", m_interrupt_gpio);
            }
        }

        // Create debounce timer
        esp_timer_create_args_t timer_args = {
            .callback = debounce_timer_callback,
            .arg = this,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "IOExpanderDebounce",
            .skip_unhandled_events = true
        };

        if (esp_timer_create(&timer_args, &m_debounce_timer) != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to create debounce timer");
        }
    }
    #endif

    ESP_LOGI(TAG, "Initialization complete");
}

IOExpander::~IOExpander()
{
    if (m_debounce_timer != nullptr)
    {
        esp_timer_delete(m_debounce_timer);
    }

    if (m_interrupt_gpio >= 0)
    {
        gpio_isr_handler_remove(m_interrupt_gpio);
    }

    if (m_semaphore_handle != nullptr)
    {
        vSemaphoreDelete(m_semaphore_handle);
    }
}

// Internal I/O operations
esp_err_t IOExpander::read_port_internal(uint8_t *value)
{
    if (!m_is_present || value == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }

    // PCF8574: Read is a simple I2C read of 1 byte
    return i2c_master_receive(m_i2c_device_handle, value, 1, -1);
}

esp_err_t IOExpander::write_port_internal(uint8_t value)
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    // PCF8574: Write is a simple I2C write of 1 byte
    return i2c_master_transmit(m_i2c_device_handle, &value, 1, -1);
}

// GPIO ISR handler (runs in ISR context)
void IRAM_ATTR IOExpander::gpio_isr_handler(void* arg)
{
    IOExpander* instance = static_cast<IOExpander*>(arg);

    // Start debounce timer (or restart if already running)
    if (instance->m_debounce_timer != nullptr)
    {
        esp_timer_start_once(instance->m_debounce_timer, CONFIG_IO_EXPANDER_DEBOUNCE_MS * 1000);
    }
}

// Debounce timer callback (runs in timer task context)
void IOExpander::debounce_timer_callback(void* arg)
{
    IOExpander* instance = static_cast<IOExpander*>(arg);
    instance->handle_interrupt();
}

// Handle interrupt (called from debounce timer)
void IOExpander::handle_interrupt()
{
    if (!m_is_present)
    {
        return;
    }

    uint8_t current_value;

    if (xSemaphoreTake(m_semaphore_handle, pdMS_TO_TICKS(10)) != pdTRUE)
    {
        ESP_LOGW(TAG, "Failed to take semaphore in interrupt handler");
        return;
    }

    // Read current port value
    if (read_port_internal(&current_value) != ESP_OK)
    {
        xSemaphoreGive(m_semaphore_handle);
        ESP_LOGE(TAG, "Failed to read port in interrupt handler");
        return;
    }

    // Detect changed pins
    uint8_t changed_pins = current_value ^ m_last_port_value;

    if (changed_pins != 0)
    {
        // Post interrupt event
        if (m_event_loop_handle != nullptr)
        {
            io_expander_interrupt_event_t int_event = {
                .port_value = current_value,
                .changed_pins = changed_pins
            };

            esp_event_post_to(m_event_loop_handle, IO_EXPANDER_EVENTS, IOExpanderInterrupt,
                            &int_event, sizeof(int_event), 0);
        }

        // Check each pin and call callbacks
        for (uint8_t pin = 0; pin < 8; pin++)
        {
            if (changed_pins & (1 << pin))
            {
                pin_state_t old_state = (m_last_port_value & (1 << pin)) ? PinHigh : PinLow;
                pin_state_t new_state = (current_value & (1 << pin)) ? PinHigh : PinLow;

                // Check interrupt mode for this pin
                bool should_trigger = false;
                switch (m_pin_interrupt_modes[pin])
                {
                    case InterruptRising:
                        should_trigger = (old_state == PinLow && new_state == PinHigh);
                        break;
                    case InterruptFalling:
                        should_trigger = (old_state == PinHigh && new_state == PinLow);
                        break;
                    case InterruptBoth:
                        should_trigger = true;
                        break;
                    default:
                        break;
                }

                if (should_trigger)
                {
                    // Call pin callback if registered
                    if (m_pin_callbacks[pin])
                    {
                        m_pin_callbacks[pin](pin, new_state);
                    }

                    // Post pin change event
                    if (m_event_loop_handle != nullptr)
                    {
                        io_expander_pin_event_t pin_event = {
                            .pin = pin,
                            .old_state = old_state,
                            .new_state = new_state,
                            .port_value = current_value
                        };

                        esp_event_post_to(m_event_loop_handle, IO_EXPANDER_EVENTS, IOExpanderPinChange,
                                        &pin_event, sizeof(pin_event), 0);
                    }
                }
            }
        }

        m_last_port_value = current_value;
    }

    xSemaphoreGive(m_semaphore_handle);
}

// Port operations
esp_err_t IOExpander::read_port(uint8_t *value)
{
    if (!m_is_present || value == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    esp_err_t result = read_port_internal(value);
    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t IOExpander::write_port(uint8_t value)
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    // Update output values and modes
    m_output_values = value;

    esp_err_t result = write_port_internal(value);
    xSemaphoreGive(m_semaphore_handle);

    return result;
}

// Pin mode operations
esp_err_t IOExpander::set_pin_mode(io_pin_t pin, pin_mode_t mode)
{
    if (!m_is_present || pin > Pin7)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    if (mode == PinInput)
    {
        m_pin_modes |= (1 << pin);
        m_output_values |= (1 << pin);  // Set high for input (weak pullup)
    }
    else
    {
        m_pin_modes &= ~(1 << pin);
        // Keep current output value
    }

    // Apply the new configuration
    uint8_t value_to_write = (m_output_values & ~m_pin_modes) | m_pin_modes;
    esp_err_t result = write_port_internal(value_to_write);

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t IOExpander::get_pin_mode(io_pin_t pin, pin_mode_t *mode)
{
    if (!m_is_present || pin > Pin7 || mode == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *mode = (m_pin_modes & (1 << pin)) ? PinInput : PinOutput;
    return ESP_OK;
}

// Pin I/O operations
esp_err_t IOExpander::digital_write(io_pin_t pin, pin_state_t state)
{
    if (!m_is_present || pin > Pin7)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    // Update output value
    if (state == PinHigh)
    {
        m_output_values |= (1 << pin);
    }
    else
    {
        m_output_values &= ~(1 << pin);
    }

    // Write the combined value (inputs stay high, outputs as set)
    uint8_t value_to_write = (m_output_values & ~m_pin_modes) | m_pin_modes;
    esp_err_t result = write_port_internal(value_to_write);

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t IOExpander::digital_read(io_pin_t pin, pin_state_t *state)
{
    if (!m_is_present || pin > Pin7 || state == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    uint8_t port_value;
    esp_err_t result = read_port_internal(&port_value);

    if (result == ESP_OK)
    {
        *state = (port_value & (1 << pin)) ? PinHigh : PinLow;
        m_last_port_value = port_value;
    }

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t IOExpander::toggle_pin(io_pin_t pin)
{
    if (!m_is_present || pin > Pin7)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    // Toggle the output value
    m_output_values ^= (1 << pin);

    // Write the combined value
    uint8_t value_to_write = (m_output_values & ~m_pin_modes) | m_pin_modes;
    esp_err_t result = write_port_internal(value_to_write);

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

// Multi-pin operations
esp_err_t IOExpander::set_pins_mode(uint8_t pin_mask, pin_mode_t mode)
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    if (mode == PinInput)
    {
        m_pin_modes |= pin_mask;
        m_output_values |= pin_mask;
    }
    else
    {
        m_pin_modes &= ~pin_mask;
    }

    uint8_t value_to_write = (m_output_values & ~m_pin_modes) | m_pin_modes;
    esp_err_t result = write_port_internal(value_to_write);

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

esp_err_t IOExpander::digital_write_mask(uint8_t pin_mask, pin_state_t state)
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);

    if (state == PinHigh)
    {
        m_output_values |= pin_mask;
    }
    else
    {
        m_output_values &= ~pin_mask;
    }

    uint8_t value_to_write = (m_output_values & ~m_pin_modes) | m_pin_modes;
    esp_err_t result = write_port_internal(value_to_write);

    xSemaphoreGive(m_semaphore_handle);

    return result;
}

// Interrupt configuration
esp_err_t IOExpander::enable_interrupt(bool enable)
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (m_interrupt_gpio < 0)
    {
        return ESP_ERR_NOT_SUPPORTED;
    }

    m_interrupt_enabled = enable;

    if (enable)
    {
        return gpio_intr_enable(m_interrupt_gpio);
    }
    else
    {
        return gpio_intr_disable(m_interrupt_gpio);
    }
}

esp_err_t IOExpander::set_pin_interrupt(io_pin_t pin, pin_interrupt_mode_t mode)
{
    if (!m_is_present || pin > Pin7)
    {
        return ESP_ERR_INVALID_ARG;
    }

    m_pin_interrupt_modes[pin] = mode;
    return ESP_OK;
}

esp_err_t IOExpander::get_pin_interrupt(io_pin_t pin, pin_interrupt_mode_t *mode)
{
    if (!m_is_present || pin > Pin7 || mode == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *mode = m_pin_interrupt_modes[pin];
    return ESP_OK;
}

// Callback registration
esp_err_t IOExpander::set_pin_callback(io_pin_t pin, pin_change_callback_t callback)
{
    if (!m_is_present || pin > Pin7)
    {
        return ESP_ERR_INVALID_ARG;
    }

    m_pin_callbacks[pin] = callback;
    return ESP_OK;
}

esp_err_t IOExpander::clear_pin_callback(io_pin_t pin)
{
    if (!m_is_present || pin > Pin7)
    {
        return ESP_ERR_INVALID_ARG;
    }

    m_pin_callbacks[pin] = nullptr;
    return ESP_OK;
}

// Event handling
esp_err_t IOExpander::register_event_handler(esp_event_handler_t event_handler, void* handler_arg)
{
    if (m_event_loop_handle == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }

    return esp_event_handler_register_with(
        m_event_loop_handle,
        IO_EXPANDER_EVENTS,
        ESP_EVENT_ANY_ID,
        event_handler,
        handler_arg
    );
}

esp_err_t IOExpander::unregister_event_handler(esp_event_handler_t event_handler)
{
    if (m_event_loop_handle == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }

    return esp_event_handler_unregister_with(
        m_event_loop_handle,
        IO_EXPANDER_EVENTS,
        ESP_EVENT_ANY_ID,
        event_handler
    );
}

// Manual poll for pin changes
esp_err_t IOExpander::poll()
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    handle_interrupt();
    return ESP_OK;
}
