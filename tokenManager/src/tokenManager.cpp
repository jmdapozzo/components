#include "tokenManager.hpp"
#include <sys/param.h>
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"

#define LOCAL_TASK_STACKSIZE 10000
#define REFRESH_RATE_OFFSET_SEC 60
#define RETRY_RATE_SECS 30
#define BUFFER_SIZE_TX 4096
#define BUFFER_SIZE_RX 2048

using namespace macdap;

static const char *TAG = "tokenManager";

static esp_err_t get_access_token(TokenManager *tokenManager)
{
    char response_buffer[BUFFER_SIZE_RX];
    int response_length = 0;
    
    esp_http_client_config_t http_client_config = {};    
    http_client_config.url = CONFIG_TOKEN_MANAGER_ENDPOINT CONFIG_TOKEN_MANAGER_API_REQUEST;
    http_client_config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    http_client_config.crt_bundle_attach = esp_crt_bundle_attach;
    http_client_config.method = HTTP_METHOD_POST;
    http_client_config.cert_pem = nullptr;
    http_client_config.buffer_size_tx = BUFFER_SIZE_TX;
    http_client_config.timeout_ms = 15000;

    esp_http_client_handle_t client = esp_http_client_init(&http_client_config);
    if (client == nullptr)
    {
        ESP_LOGE(TAG, "esp_http_client_init failed");
        return ESP_FAIL;
    }

    const char *post_data = "grant_type=client_credentials&client_id=" CONFIG_TOKEN_MANAGER_CLIENT_ID "&client_secret=" CONFIG_TOKEN_MANAGER_SECRET "&audience=" CONFIG_TOKEN_MANAGER_AUDIENCE;
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");

    if (esp_http_client_open(client, strlen(post_data)) != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_http_client_open failed");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // Write the POST data
    if (esp_http_client_write(client, post_data, strlen(post_data)) < 0)
    {
        ESP_LOGE(TAG, "esp_http_client_write failed");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // Fetch headers
    int content_length = esp_http_client_fetch_headers(client);
    int status = esp_http_client_get_status_code(client);

    if (status != 200)
    {
        ESP_LOGE(TAG, "HTTP POST request failed with status: %d", status);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // Read response data
    memset(response_buffer, 0, sizeof(response_buffer));
    if (content_length > 0)
    {
        response_length = esp_http_client_read(client, response_buffer, sizeof(response_buffer) - 1);
        
        if (response_length <= 0)
        {
            ESP_LOGE(TAG, "Failed to read response data");
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return ESP_FAIL;
        }
    }
    else
    {
        ESP_LOGE(TAG, "No response content");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // Close the connection
    esp_http_client_close(client);
    
    // Null-terminate the response buffer
    response_buffer[response_length] = '\0';

    cJSON *root = cJSON_Parse(response_buffer);
    if (root == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse JSON response");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // Check for error response
    cJSON *error_obj = cJSON_GetObjectItem(root, "error");
    if (error_obj != NULL)
    {
        const char *error_desc = "Unknown error";
        cJSON *error_desc_obj = cJSON_GetObjectItem(root, "error_description");
        if (error_desc_obj != NULL && cJSON_IsString(error_desc_obj))
        {
            error_desc = error_desc_obj->valuestring;
        }
        ESP_LOGE(TAG, "Token request error: %s", error_desc);
        cJSON_Delete(root);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // Extract token information
    cJSON *access_token_obj = cJSON_GetObjectItem(root, "access_token");
    cJSON *expires_in_obj = cJSON_GetObjectItem(root, "expires_in");
    cJSON *token_type_obj = cJSON_GetObjectItem(root, "token_type");

    if (!cJSON_IsString(access_token_obj) || !cJSON_IsNumber(expires_in_obj) || !cJSON_IsString(token_type_obj))
    {
        ESP_LOGE(TAG, "Invalid token response format");
        cJSON_Delete(root);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    const char *access_token = access_token_obj->valuestring;
    const uint32_t expires_in = (uint32_t)expires_in_obj->valueint;
    const char *token_type = token_type_obj->valuestring;
    
    esp_err_t result = tokenManager->set_token(access_token, expires_in, token_type);
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "tokenManager->set_token failed");
    }

    cJSON_Delete(root);
    esp_http_client_cleanup(client);
    
    return result;
}

static void local_task(void *parameter)
{
    TokenManager *tokenManager = (TokenManager *)parameter;

    char *task_name = pcTaskGetName(nullptr);
    ESP_LOGI(TAG, "Starting %s", task_name);

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
