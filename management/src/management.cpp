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
#define URL_MAX_LENGTH 2048

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

static esp_err_t http_event_handler(esp_http_client_event_handle_t event)
{
    Management *management = (Management *)event->user_data;

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
        if (management != nullptr)
        {
            if (!esp_http_client_is_chunked_response(event->client))
            {
                int64_t copy_length = 0;
                int64_t content_length = esp_http_client_get_content_length(event->client);

                copy_length = MIN(event->data_len, (content_length - management->m_output_buffer_length));
                if (copy_length)
                {
                    if (management->m_output_buffer_length + copy_length > sizeof(management->m_output_buffer))
                    {
                        ESP_LOGE(TAG, "Output buffer overflow from %s", url);
                        return ESP_FAIL;
                    }
                    memcpy(management->m_output_buffer + management->m_output_buffer_length, event->data, copy_length);
                }
                management->m_output_buffer_length += copy_length;
            }
            else
            {
                if (management->m_output_buffer_length + event->data_len > sizeof(management->m_output_buffer))
                {
                    ESP_LOGE(TAG, "Output buffer overflow from %s", url);
                    return ESP_FAIL;
                }
                memcpy(management->m_output_buffer + management->m_output_buffer_length, event->data, event->data_len);
                management->m_output_buffer_length += event->data_len;
            }
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        if (management != nullptr)
        {
            management->m_output_buffer_length = 0;
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

static esp_err_t do_connection(Management *management)
{
    esp_http_client_config_t http_client_config = {};
    http_client_config.url = CONFIG_REST_SERVICE_ENDPOINT_CONNECTION,
    http_client_config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    http_client_config.crt_bundle_attach = esp_crt_bundle_attach;
    http_client_config.method = HTTP_METHOD_POST,
    http_client_config.cert_pem = nullptr,
    http_client_config.event_handler = http_event_handler;
    http_client_config.user_data = management;
    http_client_config.buffer_size_tx = 4096;  // TODO reduce this
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

    ESP_LOGI(TAG, "do_connection POST at %s data %s", CONFIG_REST_SERVICE_ENDPOINT_CONNECTION, post_data);

    if (esp_http_client_perform(client) != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_http_client_perform failed");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    if (esp_http_client_get_content_length(client) > 0)
    {
        cJSON *root = cJSON_Parse(management->m_output_buffer);
        bool post_device_connection = cJSON_IsTrue(cJSON_GetObjectItem(root, "postDeviceConnection"));
        cJSON_Delete(root);

        if (!post_device_connection)
        {
            int32_t status = esp_http_client_get_status_code(client);
            ESP_LOGE(TAG, "post_device_connection failed, status=%ld", status);
            esp_http_client_cleanup(client);
            return ESP_FAIL;
        }
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
    http_client_config.event_handler = http_event_handler;
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
    esp_http_client_config_t http_client_config = {};
    http_client_config.url = CONFIG_REST_SERVICE_ENDPOINT_UPDATE,
    http_client_config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    http_client_config.crt_bundle_attach = esp_crt_bundle_attach;
    http_client_config.method = HTTP_METHOD_GET,
    http_client_config.cert_pem = nullptr,
    http_client_config.event_handler = http_event_handler;
    http_client_config.user_data = management;
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

    if (esp_http_client_perform(client) != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_http_client_perform failed");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    if (esp_http_client_get_content_length(client) > 0)
    {
        updateFirmware_t update_firmware;
        ESP_LOG_BUFFER_CHAR(TAG, management->m_output_buffer, esp_http_client_get_content_length(client));
        cJSON *root = cJSON_Parse(management->m_output_buffer);
        const char *name = cJSON_GetObjectItem(root, "name")->valuestring;
        const char *type = cJSON_GetObjectItem(root, "type")->valuestring;
        const char *date = cJSON_GetObjectItem(root, "date")->valuestring;
        const char *time = cJSON_GetObjectItem(root, "time")->valuestring;
        int size = cJSON_GetObjectItem(root, "size")->valueint;
        const char *version = cJSON_GetObjectItem(root, "version")->valuestring;
        const char *url = cJSON_GetObjectItem(root, "url")->valuestring;
        strncpy(update_firmware.name, name, sizeof(update_firmware.name) - 1);
        strncpy(update_firmware.type, type, sizeof(update_firmware.type) - 1);
        strncpy(update_firmware.date, date, sizeof(update_firmware.date) - 1);
        strncpy(update_firmware.time, time, sizeof(update_firmware.time) - 1);
        update_firmware.size = size;
        strncpy(update_firmware.version, version, sizeof(update_firmware.version) - 1);
        strncpy(update_firmware.url, url, sizeof(update_firmware.url) - 1);
        cJSON_Delete(root);

        ESP_LOGD(TAG, "name (%d): %s", strlen(update_firmware.name), update_firmware.name);
        ESP_LOGD(TAG, "type (%d): %s", strlen(update_firmware.type), update_firmware.type);
        ESP_LOGD(TAG, "date (%d): %s", strlen(update_firmware.date), update_firmware.date);
        ESP_LOGD(TAG, "time (%d): %s", strlen(update_firmware.time), update_firmware.time);
        ESP_LOGD(TAG, "size: %ld", update_firmware.size);
        ESP_LOGD(TAG, "version (%d): %s", strlen(update_firmware.version), update_firmware.version);
        ESP_LOGD(TAG, "url (%d): %s", strlen(update_firmware.url), update_firmware.url);

        if (update(update_firmware) != ESP_OK)
        {
            ESP_LOGE(TAG, "update failed");
            esp_http_client_cleanup(client);
            return ESP_FAIL;
        }
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
    Management *management = (Management *)parameter;

    vTaskDelay(pdMS_TO_TICKS(STARTUP_DELAY));

    char *task_name = pcTaskGetName(nullptr);
    ESP_LOGI(TAG, "Starting %s", task_name);

    management->m_output_buffer_length = 0;

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
            ESP_LOGE(TAG, "do_firmware_update failed");
        }
        uint16_t hour_counter = (xTaskGetTickCount() % 24) + 1;
        ESP_LOGI(TAG, "Next Update Firmware check in %d hours", hour_counter);
        while (hour_counter > 0)
        {
            hour_counter--;
            vTaskDelay(pdMS_TO_TICKS(10 * 1000)); // TODO restore this
            // vTaskDelay(pdMS_TO_TICKS(60 * 60 * 1000));
        }
    }

    vTaskDelete(nullptr);
}

Management::Management()
{
    if (!m_initialized)
    {
        esp_log_level_set(TAG, ESP_LOG_VERBOSE);
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
        } else {
            ESP_LOGW(TAG, "Management already initialized");
        }
    } else {
        ESP_LOGW(TAG, "Management already initialized");
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
