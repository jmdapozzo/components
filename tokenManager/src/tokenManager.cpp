#include "tokenManager.hpp"
#include <sys/param.h>
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"

#define LOCAL_TASK_STACKSIZE 8000
#define REFRESH_RATE_OFFSET_SEC 60
#define RETRY_RATE_SECS 30
#define MAX_HTTP_OUTPUT_BUFFER 2048

using namespace macdap;

static const char *TAG = "tokenManager";

static esp_err_t client_event_handler(esp_http_client_event_handle_t event)
{
    TokenManager *tokenManager = (TokenManager *)event->user_data;

    char url[128];
    esp_http_client_get_url(event->client, url, sizeof(url));

    switch (event->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP HTTP_EVENT_ON_CONNECTED, %s", url);
    break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", event->header_key, event->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", event->data_len);
        if (tokenManager != nullptr)
        {
            if (!esp_http_client_is_chunked_response(event->client))
            {
                int copy_length = 0;
                int content_length = esp_http_client_get_content_length(event->client);

                copy_length = MIN(event->data_len, (content_length - tokenManager->m_output_buffer_length));
                if (copy_length)
                {
                    if (tokenManager->m_output_buffer_length + copy_length > sizeof(tokenManager->m_output_buffer))
                    {
                        ESP_LOGE(TAG, "Output buffer overflow from %s", url);
                        return ESP_FAIL;
                    }
                    memcpy(tokenManager->m_output_buffer + tokenManager->m_output_buffer_length, event->data, copy_length);
                }
                tokenManager->m_output_buffer_length += copy_length;
            }
            else
            {
                if (tokenManager->m_output_buffer_length + event->data_len > sizeof(tokenManager->m_output_buffer))
                {
                    ESP_LOGE(TAG, "Output buffer overflow from %s", url);
                    return ESP_FAIL;
                }
                memcpy(tokenManager->m_output_buffer + tokenManager->m_output_buffer_length, event->data, event->data_len);
                tokenManager->m_output_buffer_length += event->data_len;
            }
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        if (tokenManager != nullptr)
        {
            tokenManager->m_output_buffer_length = 0;
        }
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP HTTP_EVENT_DISCONNECTED, %s", url);
    break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}

static esp_err_t get_access_token(TokenManager *tokenManager)
{
    esp_http_client_config_t http_client_config = {};    
    http_client_config.url = CONFIG_TOKEN_MANAGER_ENDPOINT CONFIG_TOKEN_MANAGER_API_REQUEST,
    http_client_config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    http_client_config.crt_bundle_attach = esp_crt_bundle_attach;
    http_client_config.method = HTTP_METHOD_POST,
    http_client_config.cert_pem = nullptr,
    http_client_config.event_handler = client_event_handler;
    http_client_config.user_data = tokenManager;

    esp_http_client_handle_t client = esp_http_client_init(&http_client_config);
    if (client == NULL)
    {
        ESP_LOGE(TAG, "esp_http_client_init failed");
        return ESP_FAIL;
    }

    const char *post_data = "grant_type=client_credentials&client_id=" CONFIG_TOKEN_MANAGER_CLIENT_ID "&client_secret=" CONFIG_TOKEN_MANAGER_SECRET "&audience=" CONFIG_TOKEN_MANAGER_AUDIENCE;
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");

    if (esp_http_client_perform(client) != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_http_client_perform failed");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    if (esp_http_client_get_content_length(client) > 0)
    {
        cJSON *root = cJSON_Parse(tokenManager->m_output_buffer);
        const char *access_token = cJSON_GetObjectItem(root, "access_token")->valuestring;
        const uint32_t expires_in = cJSON_GetObjectItem(root, "expires_in")->valueint;
        const char *token_type = cJSON_GetObjectItem(root, "token_type")->valuestring;
        esp_err_t result = tokenManager->set_token(access_token, expires_in, token_type);
        if (result != ESP_OK)
        {
            ESP_LOGE(TAG, "tokenManager->set_token failed");
        }

        cJSON_Delete(root);
    }

    int status = esp_http_client_get_status_code(client);
    if (status != 200)
    {
        ESP_LOGE(TAG, "HTTP POST request failed: %d", status);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    if (esp_http_client_cleanup(client))
    {
        ESP_LOGE(TAG, "esp_http_client_cleanup failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static void local_task(void *parameter)
{
    TokenManager *tokenManager = (TokenManager *)parameter;

    char *task_name = pcTaskGetName(nullptr);
    ESP_LOGI(TAG, "Starting %s", task_name);

    tokenManager->m_output_buffer_length = 0;

    while (true)
    {
        TickType_t next_refresh = RETRY_RATE_SECS;
        if (get_access_token(tokenManager) == ESP_OK)
        {
            Token_t token;
            if (tokenManager->get_token(&token) == ESP_OK)
            {
                next_refresh = (token.expires_in - REFRESH_RATE_OFFSET_SEC);
            }
            else
            {
                ESP_LOGE(TAG, "tokenManager->get_token failed");
            }
        }
        else
        {
            ESP_LOGE(TAG, "get_access_token failed");
        }
        ESP_LOGI(TAG, "Next access token refresh in %lu seconds", next_refresh);
        next_refresh = 60 * (1000 / portTICK_PERIOD_MS); //TODO restore this
        // nextRefresh = nextRefresh * (1000 / portTICK_PERIOD_MS);
        vTaskDelay(next_refresh);
    }

    vTaskDelete(NULL);
}

TokenManager::TokenManager()
{
    if (!m_initialized)
    {
        esp_log_level_set(TAG, ESP_LOG_VERBOSE);
        ESP_LOGI(TAG, "Initializing...");

        m_semaphore_handle = xSemaphoreCreateMutex();

        if (m_semaphore_handle != nullptr)
        {
            if (m_task_handle == nullptr)
            {
                if (xTaskCreate(
                        local_task,
                        TAG,
                        LOCAL_TASK_STACKSIZE,
                        this,
                        tskIDLE_PRIORITY,
                        &m_task_handle) == pdPASS)
                {
                    m_initialized = true;
                } else {
                    ESP_LOGE(TAG, "xTaskCreate failed");
                }
            } else {
                ESP_LOGW(TAG, "TokenManager already initialized");
            }
        }
        else
        {
            ESP_LOGE(TAG, "xSemaphoreCreateMutex failed");
        }
    } else {
        ESP_LOGW(TAG, "TokenManager already initialized");
    }
}

TokenManager::~TokenManager()
{
    if (m_task_handle != nullptr)
    {
        vTaskDelete(m_task_handle);
    }

    if (m_semaphore_handle != nullptr)
    {
        vSemaphoreDelete(m_semaphore_handle);
    }

    m_initialized = false;
}

esp_err_t TokenManager::set_token(const char *access_token, uint32_t expires_in, const char *token_type)
{
    esp_err_t result = ESP_OK;

    if (xSemaphoreTake(m_semaphore_handle, pdMS_TO_TICKS(10000)) == pdTRUE)
    {
        strcpy(m_token.access_token, access_token);
        m_token.expires_in = expires_in;
        strcpy(m_token.token_type, token_type);
        xSemaphoreGive(m_semaphore_handle);
    }
    else
    {
        ESP_LOGE(TAG, "xSemaphoreTake failed");
        result = ESP_ERR_TIMEOUT;
    }

    return result;
}

esp_err_t TokenManager::get_token(Token_t *token)
{
    esp_err_t result = ESP_OK;

    if (xSemaphoreTake(m_semaphore_handle, pdMS_TO_TICKS(10000)) == pdTRUE)
    {
        if (token != nullptr)
        {
            *token = m_token;
        }
        else
        {
            result = ESP_ERR_INVALID_ARG;
        }
        xSemaphoreGive(m_semaphore_handle);
    }
    else
    {
        ESP_LOGE(TAG, "xSemaphoreTake failed");
        result = ESP_ERR_TIMEOUT;
    }

    return result;
}

esp_err_t TokenManager::get_authorisation(char *authorisation, size_t maxSize)
{
    esp_err_t result = ESP_OK;

    if (xSemaphoreTake(m_semaphore_handle, pdMS_TO_TICKS(10000)) == pdTRUE)
    {
        if (authorisation != nullptr)
        {
            if (m_token.expires_in > 0)
            {
                size_t length = strlen(m_token.access_token) + strlen(m_token.token_type) + 1;
                if (length <= maxSize)
                {
                    sprintf(authorisation, "%s %s", m_token.token_type, m_token.access_token);
                }
                else
                {
                    result = ESP_ERR_INVALID_STATE;
                }
            }
            else
            {
                ESP_LOGE(TAG, "m_token.accessToken or m_token.tokenType are null!");
                result = ESP_ERR_INVALID_ARG;
            }
        }
        else
        {
            ESP_LOGE(TAG, "authorisation is null");
            result = ESP_ERR_INVALID_ARG;
        }
        xSemaphoreGive(m_semaphore_handle);
    }
    else
    {
        ESP_LOGE(TAG, "xSemaphoreTake failed");
        result = ESP_ERR_TIMEOUT;
    }

    return result;
}
