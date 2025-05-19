#include "eventLoop.hpp"
#include "esp_log.h"
#include "sys/time.h"

using namespace macdap;

static const char *TAG = "eventLoop";
esp_event_loop_handle_t m_eventLoopHandle;

void onAnyEvent(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{
    struct timeval tvNow;
    gettimeofday(&tvNow, NULL);
    int64_t receivedAt = (int64_t)tvNow.tv_sec * 1000000L + (int64_t)tvNow.tv_usec;
    
    ESP_LOGI(TAG, "Event received: base=%s, id=%ld, receivedAt=%lld", base, id, receivedAt);
}

EventLoop::EventLoop()
{
    ESP_LOGI(TAG, "Initializing...");

    esp_event_loop_args_t loopArgs = {
        .queue_size = CONFIG_EVENT_LOOP_QUEUE_SIZE,
        .task_name = TAG,
        .task_priority = CONFIG_EVENT_LOOP_LOCAL_TASK_PRIORITY,
        .task_stack_size = CONFIG_EVENT_LOOP_TASK_STACK_SIZE,
        .task_core_id = tskNO_AFFINITY
    };

    esp_event_loop_create(&loopArgs, &m_eventLoopHandle);

#if CONFIG_EVENT_LOOP_LOG_ALL_EVENTS
    esp_event_handler_register_with(m_eventLoopHandle, ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, onAnyEvent, this);
#endif
}

EventLoop::~EventLoop()
{
    esp_event_handler_unregister_with(m_eventLoopHandle, ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, onAnyEvent);
    esp_event_loop_delete(m_eventLoopHandle);
}
