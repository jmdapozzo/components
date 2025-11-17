#include "digitalInput.hpp"
#include "esp_log.h"
#include "sys/time.h"

#define ESP_INTR_FLAG_DEFAULT 0

using namespace macdap;

static const char *TAG = "digitalInput";
ESP_EVENT_DEFINE_BASE(GPIO_EVENTS);
static esp_event_loop_handle_t _event_loop_handle;

static void IRAM_ATTR isr_handler(void* arg)
{
    gpio_num_t gpioNumber = (gpio_num_t)(uintptr_t)arg;
    EdgeDetection_t edgeDetection = static_cast<EdgeDetection_t>(gpio_get_level(gpioNumber));

    esp_event_isr_post_to(_event_loop_handle, GPIO_EVENTS, gpioNumber, &edgeDetection, sizeof(EdgeDetection_t), NULL);
}

#if CONFIG_DIGITAL_INPUT_EVENT_LOG
static void on_digital_input_event(void* handler_arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    gpio_num_t gpioNumber = static_cast<gpio_num_t>(event_id);
    EdgeDetection_t edgeDetection = *static_cast<EdgeDetection_t*>(event_data);

    struct timeval tvNow;
    gettimeofday(&tvNow, NULL);
    int64_t detectedAt = (int64_t)tvNow.tv_sec * 1000000L + (int64_t)tvNow.tv_usec;

    ESP_LOGI(TAG, "GPIO %d detected %s at %lld", gpioNumber, edgeDetection == Rising ? "rising" : "falling", detectedAt);
}
#endif

DigitalInput::DigitalInput(esp_event_loop_handle_t event_loop_handle)
{
    ESP_LOGI(TAG, "Initializing...");

    if (!event_loop_handle) {
        ESP_LOGE(TAG, "Event loop handle is required but not provided");
        return;
    }

    _event_loop_handle = event_loop_handle;

#if CONFIG_DIGITAL_INPUT_EVENT_LOG
    esp_event_handler_register_with(_event_loop_handle, GPIO_EVENTS, ESP_EVENT_ANY_ID, on_digital_input_event, NULL);
#endif

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
}

DigitalInput::~DigitalInput()
{
}

esp_err_t DigitalInput::add_input(gpio_num_t gpio_num)
{
    gpio_glitch_filter_handle_t gpio_glitch_filter_handle = NULL;
    gpio_pin_glitch_filter_config_t gpio_pin_glitch_filter_config = {
        .clk_src = GLITCH_FILTER_CLK_SRC_DEFAULT,
        .gpio_num = gpio_num
    };
    esp_err_t err = ESP_OK;
    err = gpio_new_pin_glitch_filter(&gpio_pin_glitch_filter_config, &gpio_glitch_filter_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create glitch filter for GPIO %d", gpio_num);
        return err;
    }

    err = gpio_glitch_filter_enable(gpio_glitch_filter_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable glitch filter for GPIO %d", gpio_num);
        return err;
    }

    gpio_config_t gpio_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&gpio_conf);

    return gpio_isr_handler_add(gpio_num, isr_handler, (void*) gpio_num);
}

esp_err_t DigitalInput::remove_input(gpio_num_t gpio_num)
{
    return gpio_isr_handler_remove(gpio_num);
}

