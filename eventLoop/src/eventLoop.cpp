#include "eventLoop.hpp"
#include "esp_log.h"
#include "sys/time.h"

using namespace macdap;

static const char *TAG = "eventLoop";

void on_any_event(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    int64_t received_at = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
    
    ESP_LOGI(TAG, "Event received: base=%s, id=%ld, receivedAt=%lld", base, id, received_at);
}

EventLoop::EventLoop()
{
    ESP_LOGI(TAG, "Initializing...");

    esp_event_loop_args_t loop_args = {
        .queue_size = CONFIG_EVENT_LOOP_QUEUE_SIZE,
        .task_name = TAG,
        .task_priority = CONFIG_EVENT_LOOP_LOCAL_TASK_PRIORITY,
        .task_stack_size = CONFIG_EVENT_LOOP_TASK_STACK_SIZE,
        .task_core_id = tskNO_AFFINITY
    };

    esp_event_loop_create(&loop_args, &m_event_loop_handle);

#if CONFIG_EVENT_LOOP_LOG_ALL_EVENTS
    esp_event_handler_register_with(m_event_loop_handle, ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, on_any_event, this);
#endif
}

EventLoop::~EventLoop()
{
    esp_event_handler_unregister_with(m_event_loop_handle, ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, on_any_event);
    esp_event_loop_delete(m_event_loop_handle);
}
