#include <stdio.h>
#include <esp_log.h>
#include <gps.hpp>
#include <eventLoop.hpp>

static const char *TAG = "main";

// Example GPS event handler
static void gps_event_handler(void* handler_arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    switch (event_id) {
    case macdap::GpsUpdate:
        {
            macdap::gps_t *gps_data = static_cast<macdap::gps_t*>(event_data);
            ESP_LOGI(TAG, "GPS Update: Lat=%.6f, Lon=%.6f, Valid=%s", 
                     gps_data->latitude, gps_data->longitude, 
                     gps_data->valid ? "Yes" : "No");
        }
        break;
    case macdap::GpsUnknown:
        ESP_LOGW(TAG, "Unknown GPS statement: %s", (char*)event_data);
        break;
    default:
        break;
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "GPS Test Program");

    // Get event loop handle from the EventLoop component
    macdap::EventLoop &eventLoop = macdap::EventLoop::get_instance();
    esp_event_loop_handle_t event_loop_handle = eventLoop.get_event_loop_handle();

    // Initialize GPS with the event loop handle
    macdap::GPS &gps = macdap::GPS::get_instance();
    gps.set_event_loop_handle(event_loop_handle);

    // Register for GPS events in the main application
    esp_err_t ret = gps.register_event_handler(gps_event_handler, nullptr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register GPS event handler: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "GPS event handler registered successfully");
    }

    while (true) {
        // Example of accessing GPS data directly
        macdap::gps_t gps_data = gps.get_gps_data();
        if (gps_data.valid) {
            ESP_LOGI(TAG, "Direct access: GPS is valid, %d satellites in use", gps_data.sats_in_use);
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
