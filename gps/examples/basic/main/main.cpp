#include <stdio.h>
#include <esp_log.h>
#include <eventLoop.hpp>
#include <gps.hpp>

static const char *TAG = "main";

// void onGPSEvent(void* handler_arg, esp_event_base_t base, int32_t event_id, void* event_data)
// {
//     macdap::gps_t *gps = static_cast<macdap::gps_t*>(event_data);

//     switch (event_id) {
//     case macdap::GPS_UPDATE:
//         ESP_LOGI(TAG, "%d/%d/%d %d:%d:%d => \r\n"
//                  "\t\t\t\t\t\tlatitude   = %.05f°N\r\n"
//                  "\t\t\t\t\t\tlongitude  = %.05f°E\r\n"
//                  "\t\t\t\t\t\taltitude   = %.02fm\r\n"
//                  "\t\t\t\t\t\tspeed      = %fm/s",
//                  gps->date.year, gps->date.month, gps->date.day,
//                  gps->tim.hour, gps->tim.minute, gps->tim.second,
//                  gps->latitude, gps->longitude, gps->altitude, gps->speed);
//         break;
//     case macdap::GPS_UNKNOWN:
//         /* print unknown statements */
//         ESP_LOGW(TAG, "Unknown statement:%s", (char *)event_data);
//         break;
//     default:
//         break;
//     }
// }

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "GPS Test Program");

    // macdap::EventLoop &eventLoop = macdap::EventLoop::getInstance();
    // esp_event_handler_register_with(eventLoop.getEventLoopHandle(), GPS_EVENTS, ESP_EVENT_ANY_ID, onGPSEvent, NULL);
    macdap::GPS &gps = macdap::GPS::getInstance();
}
