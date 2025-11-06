#include "management.hpp"
#include <time.h>
#include <stdio.h>
#include <sys/param.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include <esp_app_desc.h>
#include "esp_mac.h"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "tokenManager.hpp"

using namespace macdap;

#define STACK_SIZE 8000
#define STARTUP_DELAY 10000
#define RETRY_DELAY 5000
#define CONFIG_REST_SERVICE_ENDPOINT_CONNECTION CONFIG_REST_SERVICE_ENDPOINT "/device/v3/connection"
#define CONFIG_REST_SERVICE_ENDPOINT_UPDATE CONFIG_REST_SERVICE_ENDPOINT "/device/v3/update"
#define FORMAT_DEVICE_CONNECTION_POST_DATA "platform_type=%s&platform_id=%016llx&title=%s&version=%s&build_number=%s"

static const char *TAG = "management";

static void get_build_number(const esp_app_desc_t *app_description, char *str, size_t size) {
    struct tm tm;
    char buf[64];

    snprintf(buf, sizeof(buf), "%s %s", app_description->date, app_description->time);
    strptime(buf, "%b %d %Y %H:%M:%S", &tm);
    snprintf(str, size, "%lld", mktime(&tm));
}

static esp_err_t init_headers(esp_http_client_handle_t client)
{
    macdap::TokenManager &tokenManager = macdap::TokenManager::get_instance();
    char authorisation[2048];
    if (tokenManager.get_authorisation(authorisation, sizeof(authorisation)) != ESP_OK)
    {
        ESP_LOGE(TAG, "tokenManager.get_authorisation failed");
        return ESP_FAIL;
    }
    
    if (esp_http_client_set_header(client, "authorization", authorisation) != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_http_client_set_header failed");
        return ESP_FAIL;
    }

    const esp_app_desc_t *app_description = esp_app_get_description();
    if (esp_http_client_set_header(client, "macdap-app-title", app_description->project_name) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set macdap-app-title header");
        return ESP_FAIL;
    }
    if (esp_http_client_set_header(client, "macdap-app-version", app_description->version) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set macdap-app-version header");
        return ESP_FAIL;
    }
    
    char build_number[32];
    get_build_number(app_description, build_number, sizeof(build_number));
    if (esp_http_client_set_header(client, "macdap-app-build-number", build_number) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set macdap-app-build-number header");
        return ESP_FAIL;
    }
    if (esp_http_client_set_header(client, "macdap-platform-type", CONFIG_APP_PLATFORM) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set macdap-platform-type header");
        return ESP_FAIL;
    }

    uint64_t mac_address = 0LL;
    if (esp_efuse_mac_get_default((uint8_t *)(&mac_address)) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get MAC address");
        return ESP_FAIL;
    }
    char mac_address_str[17];
    snprintf(mac_address_str, sizeof(mac_address_str), "%016llx", mac_address);
    if (esp_http_client_set_header(client, "macdap-platform-id", mac_address_str) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set macdap-platform-id header");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static void ota_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == ESP_HTTPS_OTA_EVENT)
    {
        switch (event_id)
        {
        case ESP_HTTPS_OTA_START:
            ESP_LOGI(TAG, "OTA started");
            break;
        case ESP_HTTPS_OTA_CONNECTED:
            ESP_LOGI(TAG, "Connected to server");
            break;
        case ESP_HTTPS_OTA_GET_IMG_DESC:
            ESP_LOGI(TAG, "Reading Image Description");
            break;
        case ESP_HTTPS_OTA_VERIFY_CHIP_ID:
            ESP_LOGI(TAG, "Verifying chip id of new image: %d", *(esp_chip_id_t *)event_data);
            break;
        case ESP_HTTPS_OTA_DECRYPT_CB:
            ESP_LOGI(TAG, "Callback to decrypt function");
            break;
        case ESP_HTTPS_OTA_WRITE_FLASH:
            ESP_LOGD(TAG, "Writing to flash: %d written", *(int *)event_data);
            break;
        case ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION:
            ESP_LOGI(TAG, "Boot partition updated. Next Partition: %d", *(esp_partition_subtype_t *)event_data);
            break;
        case ESP_HTTPS_OTA_FINISH:
            ESP_LOGI(TAG, "OTA finish");
            break;
        case ESP_HTTPS_OTA_ABORT:
            ESP_LOGI(TAG, "OTA abort");
            break;
        }
    }
}

static esp_err_t do_connection(Management *management)
{
    char response_buffer[1024];
    int response_length = 0;
    
    esp_http_client_config_t http_client_config = {};
    http_client_config.url = CONFIG_REST_SERVICE_ENDPOINT_CONNECTION;
    http_client_config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    http_client_config.crt_bundle_attach = esp_crt_bundle_attach;
    http_client_config.method = HTTP_METHOD_POST;
    http_client_config.cert_pem = nullptr;
    http_client_config.buffer_size_tx = 4096;
    http_client_config.timeout_ms = 15000;

    esp_http_client_handle_t client = esp_http_client_init(&http_client_config);
    if (client == nullptr)
    {
        ESP_LOGE(TAG, "esp_http_client_init failed");
        return ESP_FAIL;
    }

    if (init_headers(client) != ESP_OK)
    {
        ESP_LOGE(TAG, "initHeaders failed");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    const esp_app_desc_t *app_description = esp_app_get_description();
    uint64_t mac_address = 0LL;
    if (esp_efuse_mac_get_default((uint8_t *)(&mac_address)) != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_efuse_mac_get_default failed");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    char build_number[32];
    get_build_number(app_description, build_number, sizeof(build_number));
    char post_data[256];
    snprintf(post_data, sizeof(post_data), FORMAT_DEVICE_CONNECTION_POST_DATA, CONFIG_APP_PLATFORM, mac_address, app_description->project_name, app_description->version, build_number);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");

    ESP_LOGI(TAG, "Connecting to server %s...", CONFIG_REST_SERVICE_ENDPOINT_CONNECTION);

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
    }
    else
    {
        response_length = 0;
    }

    // Close the connection
    esp_http_client_close(client);
    
    if (response_length <= 0)
    {
        if (status == 200)
        {
            ESP_LOGI(TAG, "Server connection successful");
            esp_http_client_cleanup(client);
            return ESP_OK;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to read response data (length=%d, status=%d)", response_length, status);
            esp_http_client_cleanup(client);
            return ESP_FAIL;
        }
    }
    
    cJSON *root = cJSON_Parse(response_buffer);
    if (root == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse JSON response: '%s'", response_buffer);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // Check if this is an error response
    cJSON *error_obj = cJSON_GetObjectItem(root, "error");
    cJSON *message_obj = cJSON_GetObjectItem(root, "message");
    
    if (error_obj != NULL || message_obj != NULL)
    {
        // This is an error response from the server
        const char *error_message = "Unknown error";
        const char *error_code = "N/A";
        
        if (message_obj != NULL && cJSON_IsString(message_obj))
        {
            error_message = message_obj->valuestring;
        }
        
        if (error_obj != NULL)
        {
            cJSON *code_obj = cJSON_GetObjectItem(error_obj, "code");
            if (code_obj != NULL && cJSON_IsString(code_obj))
            {
                error_code = code_obj->valuestring;
            }
        }
        
        ESP_LOGW(TAG, "Server returned error - Code: %s, Message: %s", error_code, error_message);
        cJSON_Delete(root);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // Check for successful connection response
    cJSON *post_device_connection_obj = cJSON_GetObjectItem(root, "postDeviceConnection");
    bool post_device_connection = false;
    
    if (post_device_connection_obj != NULL)
    {
        post_device_connection = cJSON_IsTrue(post_device_connection_obj);
    }
    
    cJSON_Delete(root);

    if (!post_device_connection)
    {
        ESP_LOGE(TAG, "Server rejected connection");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    esp_http_client_cleanup(client);
    ESP_LOGI(TAG, "Server connection established");
    return ESP_OK;
}

static esp_err_t update(updateFirmware_t update_firmware)
{
    if (esp_event_handler_register(ESP_HTTPS_OTA_EVENT, ESP_EVENT_ANY_ID, &ota_event_handler, nullptr) != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_event_handler_register failed");
    }

    esp_http_client_config_t http_client_config = {};
    http_client_config.url = update_firmware.url,
    http_client_config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    http_client_config.crt_bundle_attach = esp_crt_bundle_attach;
    http_client_config.cert_pem = nullptr,
    http_client_config.user_agent = "ESP32-http-Update";
    http_client_config.user_data = nullptr;
    http_client_config.keep_alive_enable = true;

    esp_https_ota_config_t ota_config;
    memset(&ota_config, 0, sizeof(esp_https_ota_config_t));
    ota_config.http_config = &http_client_config;

    ESP_LOGI(TAG, "Ready to update firmware (%ld) from %s", update_firmware.size, update_firmware.url);
    esp_err_t result = esp_https_ota(&ota_config);
    if (result == ESP_OK)
    {
        ESP_LOGI(TAG, "OTA Succeed, Rebooting...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }
    else
    {
        ESP_LOGE(TAG, "esp_https_ota failed");
    }

    if (esp_event_handler_unregister(ESP_HTTPS_OTA_EVENT, ESP_EVENT_ANY_ID, &ota_event_handler) != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_event_handler_unregister failed");
    }

    return result;
}

static esp_err_t do_firmware_update(Management *management)
{
    char response_buffer[1024];
    int response_length = 0;
    
    // Clear all memory to avoid using stale data
    memset(response_buffer, 0, sizeof(response_buffer));
    
    esp_http_client_config_t http_client_config = {};
    http_client_config.url = CONFIG_REST_SERVICE_ENDPOINT_UPDATE;
    http_client_config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    http_client_config.crt_bundle_attach = esp_crt_bundle_attach;
    http_client_config.method = HTTP_METHOD_GET;
    http_client_config.cert_pem = nullptr;
    http_client_config.buffer_size_tx = 4096;

    esp_http_client_handle_t client = esp_http_client_init(&http_client_config);
    if (client == nullptr)
    {
        ESP_LOGE(TAG, "esp_http_client_init failed");
        return ESP_FAIL;
    }

    if (init_headers(client) != ESP_OK)
    {
        ESP_LOGE(TAG, "initHeaders failed");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // Use esp_http_client_open for GET request
    if (esp_http_client_open(client, 0) != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_http_client_open failed");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // Fetch headers
    int content_length = esp_http_client_fetch_headers(client);
    int status = esp_http_client_get_status_code(client);

    if (status != 200)
    {
        // Try to read error response for debugging
        if (content_length > 0)
        {
            memset(response_buffer, 0, sizeof(response_buffer));
            int error_response_length = esp_http_client_read(client, response_buffer, sizeof(response_buffer) - 1);
            if (error_response_length > 0)
            {
                response_buffer[error_response_length] = '\0';
                ESP_LOGE(TAG, "HTTP GET request for firmware update failed with status: %d, response: '%s'", status, response_buffer);
            }
            else
            {
                ESP_LOGE(TAG, "HTTP GET request for firmware update failed with status: %d (no response body)", status);
            }
        }
        else
        {
            ESP_LOGE(TAG, "HTTP GET request for firmware update failed with status: %d (no content)", status);
        }
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
            ESP_LOGE(TAG, "Failed to read firmware update response data");
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return ESP_FAIL;
        }
    }
    else
    {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return ESP_OK;
    }

    // Close the connection
    esp_http_client_close(client);
    
    // Null-terminate the response buffer
    response_buffer[response_length] = '\0';
    
    updateFirmware_t update_firmware;
    memset(&update_firmware, 0, sizeof(update_firmware));
    
    cJSON *root = cJSON_Parse(response_buffer);
    if (root == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse firmware update JSON response");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // Check if this is an error response
    cJSON *error_obj = cJSON_GetObjectItem(root, "error");
    cJSON *message_obj = cJSON_GetObjectItem(root, "message");
    
    if (error_obj != NULL || message_obj != NULL)
    {
        const char *error_message = "Unknown error";
        if (message_obj != NULL && cJSON_IsString(message_obj))
        {
            error_message = message_obj->valuestring;
        }
        ESP_LOGW(TAG, "Server returned error for firmware update: %s", error_message);
        cJSON_Delete(root);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // Safely extract firmware update information
    cJSON *name_obj = cJSON_GetObjectItem(root, "name");
    cJSON *type_obj = cJSON_GetObjectItem(root, "type");
    cJSON *date_obj = cJSON_GetObjectItem(root, "date");
    cJSON *time_obj = cJSON_GetObjectItem(root, "time");
    cJSON *size_obj = cJSON_GetObjectItem(root, "size");
    cJSON *version_obj = cJSON_GetObjectItem(root, "version");
    cJSON *url_obj = cJSON_GetObjectItem(root, "url");

    // Validate that all required fields are present and have correct types
    if (!cJSON_IsString(name_obj) || !cJSON_IsString(type_obj) || 
        !cJSON_IsString(date_obj) || !cJSON_IsString(time_obj) ||
        !cJSON_IsNumber(size_obj) || !cJSON_IsString(version_obj) || 
        !cJSON_IsString(url_obj))
    {
        ESP_LOGE(TAG, "Missing or invalid fields in firmware update response");
        cJSON_Delete(root);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // Safely copy strings with bounds checking
    strncpy(update_firmware.name, name_obj->valuestring, sizeof(update_firmware.name) - 1);
    update_firmware.name[sizeof(update_firmware.name) - 1] = '\0';
    
    strncpy(update_firmware.type, type_obj->valuestring, sizeof(update_firmware.type) - 1);
    update_firmware.type[sizeof(update_firmware.type) - 1] = '\0';
    
    strncpy(update_firmware.date, date_obj->valuestring, sizeof(update_firmware.date) - 1);
    update_firmware.date[sizeof(update_firmware.date) - 1] = '\0';
    
    strncpy(update_firmware.time, time_obj->valuestring, sizeof(update_firmware.time) - 1);
    update_firmware.time[sizeof(update_firmware.time) - 1] = '\0';
    
    update_firmware.size = (uint32_t)size_obj->valueint;
    
    strncpy(update_firmware.version, version_obj->valuestring, sizeof(update_firmware.version) - 1);
    update_firmware.version[sizeof(update_firmware.version) - 1] = '\0';
    
    strncpy(update_firmware.url, url_obj->valuestring, sizeof(update_firmware.url) - 1);
    update_firmware.url[sizeof(update_firmware.url) - 1] = '\0';
    
    cJSON_Delete(root);

    ESP_LOGI(TAG, "Starting firmware update: %s v%s (%ld bytes)", 
             update_firmware.name, update_firmware.version, update_firmware.size);

    if (update(update_firmware) != ESP_OK)
    {
        ESP_LOGE(TAG, "Firmware update failed");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    esp_http_client_cleanup(client);
    
    return ESP_OK;
}

static void local_task(void *parameter)
{
    Management *management = (Management *)parameter;

    vTaskDelay(pdMS_TO_TICKS(STARTUP_DELAY));

    char *task_name = pcTaskGetName(nullptr);
    ESP_LOGI(TAG, "Starting %s", task_name);

    bool connected = false;
    while (!connected)
    {
        if (do_connection(management) == ESP_OK)
        {
            connected = true;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to connect to server");
            vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY));
        }
    }

    while (true)
    {
        if (do_firmware_update(management) != ESP_OK)
        {
            ESP_LOGW(TAG, "Firmware update check failed, will retry later");
        }
        
        uint16_t hour_counter = (xTaskGetTickCount() % 24) + 1;
        ESP_LOGI(TAG, "Next firmware update check in %d hours", hour_counter);
        while (hour_counter > 0)
        {
            hour_counter--;
            vTaskDelay(pdMS_TO_TICKS(10 * 1000)); // TODO: restore to 60 * 60 * 1000 for production
        }
    }

    vTaskDelete(nullptr);
}

Management::Management()
{
    if (!m_initialized)
    {
        ESP_LOGI(TAG, "Initializing...");

        if (m_task_handle == nullptr)
        {
            if (xTaskCreate(
                    local_task,
                    TAG,
                    STACK_SIZE,
                    this,
                    tskIDLE_PRIORITY,
                    &m_task_handle) == pdPASS)
            {
                m_initialized = true;
            } else {
                ESP_LOGE(TAG, "Failed to create task");
            }
        }
    }
}

Management::~Management()
{
    if (m_task_handle != nullptr)
    {
        vTaskDelete(m_task_handle);
    }

    m_initialized = false;
}
