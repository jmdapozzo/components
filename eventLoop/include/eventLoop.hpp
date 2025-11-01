#pragma once

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_event.h>

namespace macdap
{
    class EventLoop
    {

    private:
        esp_event_loop_handle_t m_event_loop_handle;
        EventLoop();
        ~EventLoop();

    public:
        EventLoop(EventLoop const&) = delete;
        void operator=(EventLoop const &) = delete;
        static EventLoop &get_instance()
        {
            static EventLoop instance;
            return instance;
        }

        esp_event_loop_handle_t get_event_loop_handle() const
        {
            return m_event_loop_handle;
        }
    };
}
