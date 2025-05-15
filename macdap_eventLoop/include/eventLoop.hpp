#pragma once

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_event.h>

#ifdef __cplusplus
extern "C"
{
#endif

    namespace macdap
    {
        class EventLoop
        {

        private:
            esp_event_loop_handle_t m_eventLoopHandle;
            EventLoop();
            ~EventLoop();

        public:
            EventLoop(EventLoop const&) = delete;
            void operator=(EventLoop const &) = delete;
            static EventLoop &getInstance()
            {
                static EventLoop instance;
                return instance;
            }

            esp_event_loop_handle_t getEventLoopHandle() const
            {
                return m_eventLoopHandle;
            }
        };
    }
#ifdef __cplusplus
}
#endif