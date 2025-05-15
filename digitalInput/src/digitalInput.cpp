#include "digitalInput.hpp"
#include "esp_log.h"
#include "sys/time.h"

#define ESP_INTR_FLAG_DEFAULT 0

using namespace macdap;

static const char *TAG = "digitalInput";
ESP_EVENT_DEFINE_BASE(GPIO_EVENTS);
static esp_event_loop_handle_t _eventLoopHandle;

static void IRAM_ATTR isrHandler(void* arg)
{
    gpio_num_t gpioNumber = (gpio_num_t)(uintptr_t)arg;
    EdgeDetection_t edgeDetection = static_cast<EdgeDetection_t>(gpio_get_level(gpioNumber));

    esp_event_isr_post_to(_eventLoopHandle, GPIO_EVENTS, gpioNumber, &edgeDetection, sizeof(EdgeDetection_t), NULL);
}

#if CONFIG_DIGITAL_INPUT_EVENT_LOG
static void onDigitalInputEvent(void* handler_arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    gpio_num_t gpioNumber = static_cast<gpio_num_t>(event_id);
    EdgeDetection_t edgeDetection = *static_cast<EdgeDetection_t*>(event_data);

    struct timeval tvNow;
    gettimeofday(&tvNow, NULL);
    int64_t detectedAt = (int64_t)tvNow.tv_sec * 1000000L + (int64_t)tvNow.tv_usec;

    ESP_LOGI(TAG, "GPIO %d detected %s at %lld", gpioNumber, edgeDetection == rising ? "rising" : "falling", detectedAt);
}
#endif

DigitalInput::DigitalInput()
{
    ESP_LOGI(TAG, "Initializing...");

    macdap::EventLoop &eventLoop = macdap::EventLoop::getInstance();
    _eventLoopHandle = eventLoop.getEventLoopHandle();

#if CONFIG_DIGITAL_INPUT_EVENT_LOG
    esp_event_handler_register_with(_eventLoopHandle, GPIO_EVENTS, ESP_EVENT_ANY_ID, onDigitalInputEvent, NULL);
#endif

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
}

DigitalInput::~DigitalInput()
{
}

esp_err_t DigitalInput::addInput(gpio_num_t gpioNumber)
{
    return gpio_isr_handler_add(gpioNumber, isrHandler, (void*) gpioNumber);
}

esp_err_t DigitalInput::removeInput(gpio_num_t gpioNumber)
{
    return gpio_isr_handler_remove(gpioNumber);
}

