/**
 * @file ble.cpp
 * @brief Implementation of BLE GATT service for parameter management
 */

#include "ble.hpp"
#include <esp_log.h>
#include <cJSON.h>
#include <string.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <esp_timer.h>

static const char *TAG = "ble_param_service";

namespace macdap {

// Static instance pointer
BLE* BLE::s_instance = nullptr;

// GATT attribute database
enum {
    IDX_SVC,

    IDX_CHAR_LIST,
    IDX_CHAR_LIST_VAL,
    IDX_CHAR_LIST_CFG,

    IDX_CHAR_ACCESS,
    IDX_CHAR_ACCESS_VAL,

    IDX_CHAR_DELETE,
    IDX_CHAR_DELETE_VAL,

    IDX_CHAR_NOTIFY,
    IDX_CHAR_NOTIFY_VAL,
    IDX_CHAR_NOTIFY_CFG,

    IDX_CHAR_CONTROL,
    IDX_CHAR_CONTROL_VAL,

    IDX_NB,
};

// UUIDs
static const uint8_t service_uuid[16] = BLE_PARAM_SERVICE_UUID;
static const uint8_t char_list_uuid[16] = BLE_PARAM_LIST_CHAR_UUID;
static const uint8_t char_access_uuid[16] = BLE_PARAM_ACCESS_CHAR_UUID;
static const uint8_t char_delete_uuid[16] = BLE_PARAM_DELETE_CHAR_UUID;
static const uint8_t char_notify_uuid[16] = BLE_PARAM_NOTIFY_CHAR_UUID;
static const uint8_t char_control_uuid[16] = BLE_PARAM_CONTROL_CHAR_UUID;

// Characteristic properties
static const uint8_t char_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_notify = ESP_GATT_CHAR_PROP_BIT_NOTIFY;

// Singleton implementation
BLE& BLE::get_instance() {
    static BLE instance;
    s_instance = &instance;
    return instance;
}

// Constructor
BLE::BLE()
    : m_initialized(false),
      m_running(false),
      m_notifications_enabled(false),
      m_connected(false),
      m_conn_id(0),
      m_gatts_if(ESP_GATT_IF_NONE),
      m_service_handle(0),
      m_char_list_handle(0),
      m_char_list_val_handle(0),
      m_char_access_handle(0),
      m_char_access_val_handle(0),
      m_char_delete_handle(0),
      m_char_delete_val_handle(0),
      m_char_notify_handle(0),
      m_char_notify_val_handle(0),
      m_char_control_handle(0),
      m_char_control_val_handle(0),
      m_char_list_cccd_handle(0),
      m_char_notify_cccd_handle(0) {
    memset(m_current_param_name, 0, sizeof(m_current_param_name));
    ESP_LOGI(TAG, "BLE created");
}

// Destructor
BLE::~BLE() {
    if (m_running) {
        stop();
    }
}

// Initialize BLE stack
esp_err_t BLE::init() {
    if (m_initialized) {
        ESP_LOGW(TAG, "BLE Parameter Service already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing BLE Parameter Service...");

    // Release BLE memory if not needed
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    // Initialize BT controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "Failed to initialize BT controller: %s", esp_err_to_name(ret));
        return ret;
    }

    // Enable BLE mode
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "Failed to enable BT controller: %s", esp_err_to_name(ret));
        return ret;
    }

    // Initialize Bluedroid
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "Failed to initialize Bluedroid: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "Failed to enable Bluedroid: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register GAP callback
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "Failed to register GAP callback: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register GATT callback
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "Failed to register GATTS callback: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register application
    ret = esp_ble_gatts_app_register(0);
    if (ret) {
        ESP_LOGE(TAG, "Failed to register GATTS app: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set MTU
    ret = esp_ble_gatt_set_local_mtu(BLE_PARAM_SERVICE_MTU);
    if (ret) {
        ESP_LOGE(TAG, "Failed to set local MTU: %s", esp_err_to_name(ret));
    }

    m_initialized = true;
    ESP_LOGI(TAG, "BLE Parameter Service initialized");
    return ESP_OK;
}

// Start advertising
esp_err_t BLE::start() {
    if (!m_initialized) {
        ESP_LOGE(TAG, "BLE Parameter Service not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (m_running) {
        ESP_LOGW(TAG, "BLE Parameter Service already running");
        return ESP_OK;
    }

    // Configure advertising data
    memset(&m_adv_data, 0, sizeof(m_adv_data));
    m_adv_data.set_scan_rsp = false;
    m_adv_data.include_name = true;
    m_adv_data.include_txpower = true;
    m_adv_data.min_interval = 0x0006;
    m_adv_data.max_interval = 0x0010;
    m_adv_data.appearance = 0x00;
    m_adv_data.manufacturer_len = 0;
    m_adv_data.p_manufacturer_data = NULL;
    m_adv_data.service_data_len = 0;
    m_adv_data.p_service_data = NULL;
    m_adv_data.service_uuid_len = 16;
    m_adv_data.p_service_uuid = (uint8_t*)service_uuid;
    m_adv_data.flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);

    // Configure advertising parameters
    memset(&m_adv_params, 0, sizeof(m_adv_params));
    m_adv_params.adv_int_min = 0x20;
    m_adv_params.adv_int_max = 0x40;
    m_adv_params.adv_type = ADV_TYPE_IND;
    m_adv_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
    m_adv_params.channel_map = ADV_CHNL_ALL;
    m_adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;

    // Set device name
    const char* device_name = "FlexCore Params";
    esp_ble_gap_set_device_name(device_name);

    // Start advertising (will be done in GAP event handler)
    m_running = true;
    ESP_LOGI(TAG, "BLE Parameter Service started");
    return ESP_OK;
}

// Stop advertising
esp_err_t BLE::stop() {
    if (!m_running) {
        ESP_LOGW(TAG, "BLE Parameter Service not running");
        return ESP_OK;
    }

    esp_ble_gap_stop_advertising();
    m_running = false;
    ESP_LOGI(TAG, "BLE Parameter Service stopped");
    return ESP_OK;
}

// Check if running
bool BLE::is_running() const {
    return m_running;
}

// Enable/disable notifications
void BLE::enable_notifications(bool enable) {
    m_notifications_enabled = enable;
    ESP_LOGI(TAG, "Parameter notifications %s", enable ? "enabled" : "disabled");
}

// GAP event handler
void BLE::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "Advertising data set complete");
            if (s_instance) {
                esp_ble_gap_start_advertising(&s_instance->m_adv_params);
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Advertising start failed");
            } else {
                ESP_LOGI(TAG, "Advertising started");
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            ESP_LOGI(TAG, "Advertising stopped");
            break;
        default:
            break;
    }
}

// GATTS event handler
void BLE::gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    if (!s_instance) return;

    switch (event) {
        case ESP_GATTS_REG_EVT:
            s_instance->handle_gatts_register_event(gatts_if, param);
            break;
        case ESP_GATTS_CREATE_EVT:
            ESP_LOGI(TAG, "Service created, status: %d, service_handle: %d", param->create.status, param->create.service_handle);
            if (param->create.status == ESP_GATT_OK) {
                s_instance->m_service_handle = param->create.service_handle;
                // Start service
                esp_ble_gatts_start_service(s_instance->m_service_handle);
                // Add characteristics
                s_instance->add_characteristics();
            }
            break;
        case ESP_GATTS_ADD_CHAR_EVT:
            ESP_LOGI(TAG, "Characteristic added, status: %d, attr_handle: %d, service_handle: %d",
                    param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
            // Store characteristic handles based on UUID
            // We'll identify them by the order they're added
            static int char_index = 0;
            if (param->add_char.status == ESP_GATT_OK) {
                switch (char_index) {
                    case 0: s_instance->m_char_list_val_handle = param->add_char.attr_handle; break;
                    case 1: s_instance->m_char_access_val_handle = param->add_char.attr_handle; break;
                    case 2: s_instance->m_char_delete_val_handle = param->add_char.attr_handle; break;
                    case 3: s_instance->m_char_notify_val_handle = param->add_char.attr_handle; break;
                    case 4: s_instance->m_char_control_val_handle = param->add_char.attr_handle; break;
                }
                char_index++;
            }
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGI(TAG, "Service started, status: %d", param->start.status);
            // Set advertising data now
            esp_ble_gap_config_adv_data(&s_instance->m_adv_data);
            break;
        case ESP_GATTS_READ_EVT:
            s_instance->handle_gatts_read_event(gatts_if, param);
            break;
        case ESP_GATTS_WRITE_EVT:
            s_instance->handle_gatts_write_event(gatts_if, param);
            break;
        case ESP_GATTS_CONNECT_EVT:
            s_instance->handle_gatts_connect_event(gatts_if, param);
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            s_instance->handle_gatts_disconnect_event(gatts_if, param);
            break;
        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(TAG, "MTU negotiated: %d", param->mtu.mtu);
            break;
        default:
            break;
    }
}

// Handle GATTS register event
void BLE::handle_gatts_register_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    if (param->reg.status != ESP_GATT_OK) {
        ESP_LOGE(TAG, "Registration failed, status: %d", param->reg.status);
        return;
    }

    m_gatts_if = gatts_if;
    ESP_LOGI(TAG, "GATTS registered, app_id: %d", param->reg.app_id);

    // Create service
    esp_gatt_srvc_id_t service_id;
    service_id.is_primary = true;
    service_id.id.inst_id = 0x00;
    service_id.id.uuid.len = ESP_UUID_LEN_128;
    memcpy(service_id.id.uuid.uuid.uuid128, service_uuid, 16);

    esp_ble_gatts_create_service(m_gatts_if, &service_id, 20);  // 20 handles
}

// Add all characteristics to the service
esp_err_t BLE::add_characteristics() {
    ESP_LOGI(TAG, "Adding characteristics to service");

    // 1. Parameter List characteristic (Read, Notify)
    {
        esp_bt_uuid_t uuid;
        uuid.len = ESP_UUID_LEN_128;
        memcpy(uuid.uuid.uuid128, char_list_uuid, 16);
        esp_ble_gatts_add_char(m_service_handle, &uuid,
                              ESP_GATT_PERM_READ,
                              char_prop_read_notify,
                              NULL, NULL);
    }

    // 2. Parameter Access characteristic (Read, Write)
    {
        esp_bt_uuid_t uuid;
        uuid.len = ESP_UUID_LEN_128;
        memcpy(uuid.uuid.uuid128, char_access_uuid, 16);
        esp_ble_gatts_add_char(m_service_handle, &uuid,
                              ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                              char_prop_read_write,
                              NULL, NULL);
    }

    // 3. Parameter Delete characteristic (Write)
    {
        esp_bt_uuid_t uuid;
        uuid.len = ESP_UUID_LEN_128;
        memcpy(uuid.uuid.uuid128, char_delete_uuid, 16);
        esp_ble_gatts_add_char(m_service_handle, &uuid,
                              ESP_GATT_PERM_WRITE,
                              char_prop_write,
                              NULL, NULL);
    }

    // 4. Parameter Notify characteristic (Notify)
    {
        esp_bt_uuid_t uuid;
        uuid.len = ESP_UUID_LEN_128;
        memcpy(uuid.uuid.uuid128, char_notify_uuid, 16);
        esp_ble_gatts_add_char(m_service_handle, &uuid,
                              ESP_GATT_PERM_READ,
                              char_prop_notify,
                              NULL, NULL);
    }

    // 5. Control characteristic (Write)
    {
        esp_bt_uuid_t uuid;
        uuid.len = ESP_UUID_LEN_128;
        memcpy(uuid.uuid.uuid128, char_control_uuid, 16);
        esp_ble_gatts_add_char(m_service_handle, &uuid,
                              ESP_GATT_PERM_WRITE,
                              char_prop_write,
                              NULL, NULL);
    }

    return ESP_OK;
}

// Handle GATTS read event
void BLE::handle_gatts_read_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    ESP_LOGI(TAG, "Read event, handle: %d", param->read.handle);

    esp_gatt_rsp_t rsp;
    memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
    rsp.attr_value.handle = param->read.handle;

    // Determine which characteristic is being read
    if (param->read.handle == m_char_list_val_handle) {
        // Parameter List characteristic
        char* json = encode_parameter_list();
        if (json) {
            size_t len = strlen(json);
            if (len > ESP_GATT_MAX_ATTR_LEN) {
                len = ESP_GATT_MAX_ATTR_LEN;
            }
            memcpy(rsp.attr_value.value, json, len);
            rsp.attr_value.len = len;
            free(json);
        }
    } else if (param->read.handle == m_char_access_val_handle) {
        // Parameter Access characteristic - return current parameter value
        if (strlen(m_current_param_name) > 0) {
            char* json = encode_parameter(m_current_param_name);
            if (json) {
                size_t len = strlen(json);
                if (len > ESP_GATT_MAX_ATTR_LEN) {
                    len = ESP_GATT_MAX_ATTR_LEN;
                }
                memcpy(rsp.attr_value.value, json, len);
                rsp.attr_value.len = len;
                free(json);
            }
        } else {
            // No parameter selected
            const char* error = "{\"status\":\"error\",\"message\":\"No parameter selected\"}";
            size_t len = strlen(error);
            memcpy(rsp.attr_value.value, error, len);
            rsp.attr_value.len = len;
        }
    }

    esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
}

// Handle GATTS write event
void BLE::handle_gatts_write_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    ESP_LOGI(TAG, "Write event, handle: %d, len: %d", param->write.handle, param->write.len);

    esp_gatt_status_t status = ESP_GATT_OK;

    // Determine which characteristic is being written
    if (param->write.handle == m_char_access_val_handle) {
        status = handle_access_write(param->write.value, param->write.len);
    } else if (param->write.handle == m_char_delete_val_handle) {
        status = handle_delete_write(param->write.value, param->write.len);
    } else if (param->write.handle == m_char_control_val_handle) {
        status = handle_control_write(param->write.value, param->write.len);
    }

    if (param->write.need_rsp) {
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
    }
}

// Handle connect event
void BLE::handle_gatts_connect_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    m_connected = true;
    m_conn_id = param->connect.conn_id;
    ESP_LOGI(TAG, "Client connected, conn_id: %d", m_conn_id);
}

// Handle disconnect event
void BLE::handle_gatts_disconnect_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    m_connected = false;
    ESP_LOGI(TAG, "Client disconnected, conn_id: %d", param->disconnect.conn_id);

    // Restart advertising
    if (m_running) {
        esp_ble_gap_start_advertising(&m_adv_params);
    }
}

// Handle Parameter List read
esp_err_t BLE::handle_list_read(esp_ble_gatts_cb_param_t *param) {
    char* json = encode_parameter_list();
    if (!json) {
        return ESP_ERR_NO_MEM;
    }

    // Send response handled in read event handler
    free(json);
    return ESP_OK;
}

// Handle Parameter Access read
esp_err_t BLE::handle_access_read(esp_ble_gatts_cb_param_t *param) {
    if (strlen(m_current_param_name) == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    char* json = encode_parameter(m_current_param_name);
    if (!json) {
        return ESP_ERR_NO_MEM;
    }

    free(json);
    return ESP_OK;
}

// Handle Parameter Access write (set parameter or select for read)
esp_gatt_status_t BLE::handle_access_write(const uint8_t *data, uint16_t len) {
    if (!data || len == 0) {
        return ESP_GATT_INVALID_ATTR_LEN;
    }

    // Null terminate
    char buffer[512];
    if (len >= sizeof(buffer)) {
        len = sizeof(buffer) - 1;
    }
    memcpy(buffer, data, len);
    buffer[len] = '\0';

    // Try to parse as JSON (set request)
    cJSON *json = cJSON_Parse(buffer);
    if (json) {
        // This is a set request
        cJSON *name_item = cJSON_GetObjectItem(json, "name");
        cJSON *value_item = cJSON_GetObjectItem(json, "value");

        if (!name_item || !cJSON_IsString(name_item)) {
            cJSON_Delete(json);
            return ESP_GATT_INVALID_ATTR_LEN;
        }

        const char* param_name = name_item->valuestring;
        ParameterManager &paramMgr = ParameterManager::get_instance();

        if (!paramMgr.exists(param_name)) {
            cJSON_Delete(json);
            return ESP_GATT_INVALID_HANDLE;
        }

        // Get parameter type
        ParameterManager::ParameterType type;
        esp_err_t err = paramMgr.get_type(param_name, &type);
        if (err != ESP_OK) {
            cJSON_Delete(json);
            return ESP_GATT_ERROR;
        }

        // Set value based on type
        switch (type) {
            case ParameterManager::ParameterType::String:
                if (cJSON_IsString(value_item)) {
                    err = paramMgr.set_string(param_name, value_item->valuestring);
                } else {
                    err = ESP_ERR_INVALID_ARG;
                }
                break;
            case ParameterManager::ParameterType::Integer:
                if (cJSON_IsNumber(value_item)) {
                    err = paramMgr.set_int(param_name, (int32_t)value_item->valuedouble);
                } else {
                    err = ESP_ERR_INVALID_ARG;
                }
                break;
            case ParameterManager::ParameterType::Float:
                if (cJSON_IsNumber(value_item)) {
                    err = paramMgr.set_float(param_name, (float)value_item->valuedouble);
                } else {
                    err = ESP_ERR_INVALID_ARG;
                }
                break;
            case ParameterManager::ParameterType::Boolean:
                if (cJSON_IsBool(value_item)) {
                    err = paramMgr.set_bool(param_name, cJSON_IsTrue(value_item));
                } else {
                    err = ESP_ERR_INVALID_ARG;
                }
                break;
        }

        cJSON_Delete(json);
        return err == ESP_OK ? ESP_GATT_OK : ESP_GATT_ERROR;
    } else {
        // This is a simple string - parameter name for read
        strncpy(m_current_param_name, buffer, BLE_PARAM_MAX_PARAM_NAME_LEN - 1);
        m_current_param_name[BLE_PARAM_MAX_PARAM_NAME_LEN - 1] = '\0';
        ESP_LOGI(TAG, "Parameter selected for read: %s", m_current_param_name);
        return ESP_GATT_OK;
    }
}

// Handle Parameter Delete write
esp_gatt_status_t BLE::handle_delete_write(const uint8_t *data, uint16_t len) {
    if (!data || len == 0) {
        return ESP_GATT_INVALID_ATTR_LEN;
    }

    char buffer[512];
    if (len >= sizeof(buffer)) {
        len = sizeof(buffer) - 1;
    }
    memcpy(buffer, data, len);
    buffer[len] = '\0';

    cJSON *json = cJSON_Parse(buffer);
    if (!json) {
        return ESP_GATT_INVALID_ATTR_LEN;
    }

    cJSON *name_item = cJSON_GetObjectItem(json, "name");
    if (!name_item || !cJSON_IsString(name_item)) {
        cJSON_Delete(json);
        return ESP_GATT_INVALID_ATTR_LEN;
    }

    const char* param_name = name_item->valuestring;
    ParameterManager &paramMgr = ParameterManager::get_instance();

    esp_err_t err = paramMgr.delete_parameter(param_name);
    cJSON_Delete(json);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Parameter deleted: %s", param_name);
        return ESP_GATT_OK;
    } else {
        return ESP_GATT_ERROR;
    }
}

// Handle Control write
esp_gatt_status_t BLE::handle_control_write(const uint8_t *data, uint16_t len) {
    if (!data || len == 0) {
        return ESP_GATT_INVALID_ATTR_LEN;
    }

    char buffer[512];
    if (len >= sizeof(buffer)) {
        len = sizeof(buffer) - 1;
    }
    memcpy(buffer, data, len);
    buffer[len] = '\0';

    cJSON *json = cJSON_Parse(buffer);
    if (!json) {
        return ESP_GATT_INVALID_ATTR_LEN;
    }

    cJSON *command_item = cJSON_GetObjectItem(json, "command");
    if (!command_item || !cJSON_IsString(command_item)) {
        cJSON_Delete(json);
        return ESP_GATT_INVALID_ATTR_LEN;
    }

    const char* command = command_item->valuestring;
    ESP_LOGI(TAG, "Control command received: %s", command);

    esp_gatt_status_t result = ESP_GATT_OK;

    if (strcmp(command, "refresh_list") == 0) {
        // Trigger notification with updated list
        if (m_notifications_enabled && m_connected) {
            char* json_list = encode_parameter_list();
            if (json_list) {
                send_notification(m_char_list_val_handle, (uint8_t*)json_list, strlen(json_list));
                free(json_list);
            }
        }
    } else if (strcmp(command, "restart") == 0) {
        ESP_LOGI(TAG, "System restart requested via BLE");
        // Delay restart to allow response to be sent
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    } else if (strcmp(command, "factory_reset") == 0) {
        ESP_LOGI(TAG, "Factory reset requested via BLE");
        // Erase NVS
        esp_err_t err = nvs_flash_erase();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to erase NVS: %s", esp_err_to_name(err));
            result = ESP_GATT_ERROR;
        } else {
            // Restart device
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
        }
    } else if (strcmp(command, "reset_to_defaults") == 0) {
        ParameterManager &paramMgr = ParameterManager::get_instance();
        esp_err_t err = paramMgr.reset_all_to_defaults();
        if (err != ESP_OK) {
            result = ESP_GATT_ERROR;
        }
    } else {
        result = ESP_GATT_INVALID_ATTR_LEN;
    }

    cJSON_Delete(json);
    return result;
}

// Parameter change callback
void BLE::parameter_change_callback(const char* id, void* user_data) {
    BLE* service = static_cast<BLE*>(user_data);
    if (service && service->m_notifications_enabled && service->m_connected) {
        service->send_parameter_notification(id);
    }
}

// Send parameter change notification
void BLE::send_parameter_notification(const char* param_name) {
    if (!m_connected || !m_notifications_enabled) {
        return;
    }

    // Create notification JSON
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "event", "parameter_changed");
    cJSON_AddStringToObject(root, "name", param_name);
    cJSON_AddNumberToObject(root, "timestamp", esp_timer_get_time() / 1000000);

    // Get parameter type and value
    ParameterManager &paramMgr = ParameterManager::get_instance();
    ParameterManager::ParameterType type;
    if (paramMgr.get_type(param_name, &type) == ESP_OK) {
        const char* type_str = "unknown";
        switch (type) {
            case ParameterManager::ParameterType::String:
                type_str = "string";
                char str_val[256];
                if (paramMgr.get_string(param_name, str_val, sizeof(str_val)) == ESP_OK) {
                    cJSON_AddStringToObject(root, "value", str_val);
                }
                break;
            case ParameterManager::ParameterType::Integer:
                type_str = "integer";
                int32_t int_val;
                if (paramMgr.get_int(param_name, &int_val) == ESP_OK) {
                    cJSON_AddNumberToObject(root, "value", int_val);
                }
                break;
            case ParameterManager::ParameterType::Float:
                type_str = "float";
                float float_val;
                if (paramMgr.get_float(param_name, &float_val) == ESP_OK) {
                    cJSON_AddNumberToObject(root, "value", float_val);
                }
                break;
            case ParameterManager::ParameterType::Boolean:
                type_str = "boolean";
                bool bool_val;
                if (paramMgr.get_bool(param_name, &bool_val) == ESP_OK) {
                    cJSON_AddBoolToObject(root, "value", bool_val);
                }
                break;
        }
        cJSON_AddStringToObject(root, "type", type_str);
    }

    char* json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        send_notification(m_char_notify_val_handle, (uint8_t*)json_str, strlen(json_str));
        free(json_str);
    }
    cJSON_Delete(root);
}

// Send notification
void BLE::send_notification(uint16_t char_handle, const uint8_t* data, uint16_t len) {
    if (!m_connected) {
        return;
    }

    esp_ble_gatts_send_indicate(m_gatts_if, m_conn_id, char_handle, len, (uint8_t*)data, false);
}

// Encode parameter list as JSON
char* BLE::encode_parameter_list() {
    ParameterManager &paramMgr = ParameterManager::get_instance();

    char param_ids[100][33];
    size_t count = 0;

    esp_err_t err = paramMgr.list_parameters(param_ids, 100, &count);
    if (err != ESP_OK) {
        return nullptr;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "count", count);

    cJSON *params_array = cJSON_CreateArray();
    for (size_t i = 0; i < count; i++) {
        cJSON *param_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(param_obj, "name", param_ids[i]);

        ParameterManager::ParameterType type;
        if (paramMgr.get_type(param_ids[i], &type) == ESP_OK) {
            const char* type_str = "unknown";
            switch (type) {
                case ParameterManager::ParameterType::String: type_str = "string"; break;
                case ParameterManager::ParameterType::Integer: type_str = "integer"; break;
                case ParameterManager::ParameterType::Float: type_str = "float"; break;
                case ParameterManager::ParameterType::Boolean: type_str = "boolean"; break;
            }
            cJSON_AddStringToObject(param_obj, "type", type_str);
        }

        cJSON_AddItemToArray(params_array, param_obj);
    }

    cJSON_AddItemToObject(root, "parameters", params_array);

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return json_str;
}

// Encode single parameter as JSON
char* BLE::encode_parameter(const char* name) {
    ParameterManager &paramMgr = ParameterManager::get_instance();

    if (!paramMgr.exists(name)) {
        return nullptr;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "name", name);
    cJSON_AddStringToObject(root, "status", "ok");

    ParameterManager::ParameterType type;
    if (paramMgr.get_type(name, &type) == ESP_OK) {
        const char* type_str = "unknown";
        switch (type) {
            case ParameterManager::ParameterType::String:
                type_str = "string";
                char str_val[256];
                if (paramMgr.get_string(name, str_val, sizeof(str_val)) == ESP_OK) {
                    cJSON_AddStringToObject(root, "value", str_val);
                }
                break;
            case ParameterManager::ParameterType::Integer:
                type_str = "integer";
                int32_t int_val;
                if (paramMgr.get_int(name, &int_val) == ESP_OK) {
                    cJSON_AddNumberToObject(root, "value", int_val);
                }
                break;
            case ParameterManager::ParameterType::Float:
                type_str = "float";
                float float_val;
                if (paramMgr.get_float(name, &float_val) == ESP_OK) {
                    cJSON_AddNumberToObject(root, "value", float_val);
                }
                break;
            case ParameterManager::ParameterType::Boolean:
                type_str = "boolean";
                bool bool_val;
                if (paramMgr.get_bool(name, &bool_val) == ESP_OK) {
                    cJSON_AddBoolToObject(root, "value", bool_val);
                }
                break;
        }
        cJSON_AddStringToObject(root, "type", type_str);
    }

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return json_str;
}

} // namespace macdap
