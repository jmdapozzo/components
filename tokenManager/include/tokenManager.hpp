#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

namespace macdap
{
    typedef struct xTOKEN_TOKEN
    {
        char access_token[2048];
        uint32_t expires_in;
        char token_type[64];
    } Token_t;

    class TokenManager
    {

    private:
        bool m_initialized = false;
        TaskHandle_t m_task_handle = nullptr;
        SemaphoreHandle_t m_semaphore_handle;
        Token_t m_token;
        TokenManager();
        ~TokenManager();

    public:
        TokenManager(TokenManager const&) = delete;
        void operator=(TokenManager const &) = delete;
        char m_output_buffer[2048];
        int64_t m_output_buffer_length;
        static TokenManager &get_instance()
        {
            static TokenManager instance;
            return instance;
        }

        esp_err_t set_token(const char *access_token, uint32_t expires_in, const char *token_type);
        esp_err_t get_token(Token_t *token);
        esp_err_t get_authorisation(char *authorisation, size_t max_size);
    };
}
