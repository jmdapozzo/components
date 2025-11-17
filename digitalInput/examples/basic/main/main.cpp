#include <stdio.h>
#include <esp_log.h>
#include <digitalInput.hpp>
#include <eventLoop.hpp>

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Digital Input Test Program");

    // Get event loop handle from the EventLoop component
    macdap::EventLoop &eventLoop = macdap::EventLoop::get_instance();
    esp_event_loop_handle_t event_loop_handle = eventLoop.get_event_loop_handle();

    macdap::DigitalInput &digitalInput = macdap::DigitalInput::get_instance(event_loop_handle);

    digitalInput.add_input(static_cast<gpio_num_t>(43));
    digitalInput.add_input(static_cast<gpio_num_t>(44));
    // digitalInput.add_input(static_cast<gpio_num_t>(0));
    // digitalInput.add_input(static_cast<gpio_num_t>(5));
    // digitalInput.add_input(static_cast<gpio_num_t>(6));
    // digitalInput.add_input(static_cast<gpio_num_t>(7));

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
