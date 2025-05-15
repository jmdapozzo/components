#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C"
{
#endif

    namespace macdap
    {
        typedef struct xTOKEN_TOKEN
        {
            char accessToken[2048];
            uint32_t expiresIn;
            char tokenType[64];
        } Token_t;

        class TokenManager
        {

        private:
            TaskHandle_t m_taskHandle = nullptr;
            SemaphoreHandle_t m_semaphoreHandle;
            Token_t m_token;
            TokenManager();
            ~TokenManager();

        public:
            TokenManager(TokenManager const&) = delete;
            void operator=(TokenManager const &) = delete;
            char m_outputBuffer[2048];
            int64_t m_outputLength;
            static TokenManager &getInstance()
            {
                static TokenManager instance;
                return instance;
            }

            esp_err_t setToken(const char *accessToken, uint32_t expiresIn, const char *tokenType);
            esp_err_t getToken(Token_t *token);
            esp_err_t getAuthorisation(char *authorisation, size_t maxSize);
        };
    }
#ifdef __cplusplus
}
#endif