/**
 * @file parameterManager.cpp
 * @brief Parameter management implementation
 */

#include "parameterManager.hpp"
#include <esp_log.h>
#include <nvs_flash.h>
#include <cstring>
#include <cstdlib>

static const char *TAG = "ParameterManager";

#define MAX_ID_LENGTH 32
#define MAX_NVS_KEY_LENGTH 15  // NVS key limitation

namespace macdap {

ParameterManager& ParameterManager::get_instance()
{
    static ParameterManager instance;
    return instance;
}

ParameterManager::ParameterManager()
    : m_mutex(nullptr)
    , m_nvs_handle(0)
{
    // Create mutex for thread-safe access
    m_mutex = xSemaphoreCreateMutex();
    if (m_mutex == nullptr) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return;
    }

    // Open NVS namespace
    esp_err_t err = nvs_open(CONFIG_PARAMETER_MANAGER_NVS_NAMESPACE, NVS_READWRITE, &m_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        m_nvs_handle = 0;
    } else {
        ESP_LOGI(TAG, "Parameter Manager initialized");
    }
}

ParameterManager::~ParameterManager()
{
    if (m_nvs_handle != 0) {
        nvs_close(m_nvs_handle);
    }

    // Free default value allocations
    for (auto& pair : m_parameters) {
        if (pair.second.default_value != nullptr) {
            free(pair.second.default_value);
        }
    }

    if (m_mutex != nullptr) {
        vSemaphoreDelete(m_mutex);
    }
}

bool ParameterManager::validate_id(const char* id)
{
    if (id == nullptr || strlen(id) == 0 || strlen(id) > MAX_ID_LENGTH) {
        return false;
    }

    // Check for valid characters (alphanumeric, underscore, hyphen)
    for (size_t i = 0; i < strlen(id); i++) {
        char c = id[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-')) {
            return false;
        }
    }

    return true;
}

void ParameterManager::create_nvs_key(const char* id, char* key)
{
    // NVS keys are limited to 15 characters
    // Use a hash-based approach for long IDs
    size_t id_len = strlen(id);

    if (id_len <= MAX_NVS_KEY_LENGTH) {
        strncpy(key, id, MAX_NVS_KEY_LENGTH);
        key[MAX_NVS_KEY_LENGTH] = '\0';
    } else {
        // Create a shorter key by hashing
        // Simple hash: use first 10 chars + 4 char hash + null terminator
        uint32_t hash = 0;
        for (size_t i = 0; i < id_len; i++) {
            hash = hash * 31 + id[i];
        }
        snprintf(key, MAX_NVS_KEY_LENGTH + 1, "%.10s%04x", id, (unsigned int)(hash & 0xFFFF));
    }
}

esp_err_t ParameterManager::create_parameter(const char* id, ParameterType type, const void* default_value)
{
    if (!validate_id(id)) {
        ESP_LOGE(TAG, "Invalid parameter ID: %s", id ? id : "null");
        return ESP_ERR_INVALID_ARG;
    }

    if (m_nvs_handle == 0) {
        ESP_LOGE(TAG, "NVS not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_mutex, portMAX_DELAY);

    // Check if parameter already exists
    auto it = m_parameters.find(id);
    if (it != m_parameters.end()) {
        // Update default value if provided
        if (default_value != nullptr) {
            if (it->second.default_value != nullptr) {
                free(it->second.default_value);
            }

            // Allocate and copy default value based on type
            switch (type) {
                case ParameterType::String: {
                    const char* str_val = static_cast<const char*>(default_value);
                    it->second.default_value = strdup(str_val);
                    break;
                }
                case ParameterType::Integer: {
                    it->second.default_value = malloc(sizeof(int32_t));
                    memcpy(it->second.default_value, default_value, sizeof(int32_t));
                    break;
                }
                case ParameterType::Float: {
                    it->second.default_value = malloc(sizeof(float));
                    memcpy(it->second.default_value, default_value, sizeof(float));
                    break;
                }
                case ParameterType::Boolean: {
                    it->second.default_value = malloc(sizeof(bool));
                    memcpy(it->second.default_value, default_value, sizeof(bool));
                    break;
                }
            }
        }
        xSemaphoreGive(m_mutex);
        return ESP_OK;
    }

    // Check if we've reached the maximum number of parameters
    #ifdef CONFIG_PARAMETER_MANAGER_MAX_PARAMS
    if (m_parameters.size() >= CONFIG_PARAMETER_MANAGER_MAX_PARAMS) {
        xSemaphoreGive(m_mutex);
        ESP_LOGE(TAG, "Maximum number of parameters (%d) reached", CONFIG_PARAMETER_MANAGER_MAX_PARAMS);
        return ESP_ERR_NO_MEM;
    }
    #endif

    // Create new parameter
    ParameterInfo info;
    info.type = type;
    info.default_value = nullptr;
    // callbacks vector is empty by default

    // Copy default value if provided
    if (default_value != nullptr) {
        switch (type) {
            case ParameterType::String: {
                const char* str_val = static_cast<const char*>(default_value);
                info.default_value = strdup(str_val);
                break;
            }
            case ParameterType::Integer: {
                info.default_value = malloc(sizeof(int32_t));
                memcpy(info.default_value, default_value, sizeof(int32_t));
                break;
            }
            case ParameterType::Float: {
                info.default_value = malloc(sizeof(float));
                memcpy(info.default_value, default_value, sizeof(float));
                break;
            }
            case ParameterType::Boolean: {
                info.default_value = malloc(sizeof(bool));
                memcpy(info.default_value, default_value, sizeof(bool));
                break;
            }
        }
    }

    m_parameters[id] = info;

    xSemaphoreGive(m_mutex);

    #ifdef CONFIG_PARAMETER_MANAGER_ENABLE_LOGGING
    ESP_LOGI(TAG, "Created parameter: %s (type: %d)", id, static_cast<int>(type));
    #endif
    return ESP_OK;
}

esp_err_t ParameterManager::delete_parameter(const char* id)
{
    if (!validate_id(id)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (m_nvs_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_mutex, portMAX_DELAY);

    auto it = m_parameters.find(id);
    if (it == m_parameters.end()) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    // Free default value if allocated
    if (it->second.default_value != nullptr) {
        free(it->second.default_value);
    }

    // Delete from NVS
    char nvs_key[MAX_NVS_KEY_LENGTH + 1];
    create_nvs_key(id, nvs_key);
    nvs_erase_key(m_nvs_handle, nvs_key);
    nvs_commit(m_nvs_handle);

    // Remove from map
    m_parameters.erase(it);

    xSemaphoreGive(m_mutex);

    #ifdef CONFIG_PARAMETER_MANAGER_ENABLE_LOGGING
    ESP_LOGI(TAG, "Deleted parameter: %s", id);
    #endif
    return ESP_OK;
}

bool ParameterManager::exists(const char* id)
{
    if (!validate_id(id)) {
        return false;
    }

    xSemaphoreTake(m_mutex, portMAX_DELAY);
    bool result = (m_parameters.find(id) != m_parameters.end());
    xSemaphoreGive(m_mutex);

    return result;
}

esp_err_t ParameterManager::get_type(const char* id, ParameterType* type)
{
    if (!validate_id(id) || type == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_mutex, portMAX_DELAY);

    auto it = m_parameters.find(id);
    if (it == m_parameters.end()) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    *type = it->second.type;
    xSemaphoreGive(m_mutex);

    return ESP_OK;
}

esp_err_t ParameterManager::get_string(const char* id, char* value, size_t max_len)
{
    if (!validate_id(id) || value == nullptr || max_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (m_nvs_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_mutex, portMAX_DELAY);

    auto it = m_parameters.find(id);
    if (it == m_parameters.end()) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    if (it->second.type != ParameterType::String) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_INVALID_ARG;
    }

    char nvs_key[MAX_NVS_KEY_LENGTH + 1];
    create_nvs_key(id, nvs_key);

    size_t required_size = max_len;
    esp_err_t err = nvs_get_str(m_nvs_handle, nvs_key, value, &required_size);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // Use default value if available
        if (it->second.default_value != nullptr) {
            strncpy(value, static_cast<const char*>(it->second.default_value), max_len - 1);
            value[max_len - 1] = '\0';
            err = ESP_OK;
        }
    }

    xSemaphoreGive(m_mutex);
    return err;
}

esp_err_t ParameterManager::get_int(const char* id, int32_t* value)
{
    if (!validate_id(id) || value == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    if (m_nvs_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_mutex, portMAX_DELAY);

    auto it = m_parameters.find(id);
    if (it == m_parameters.end()) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    if (it->second.type != ParameterType::Integer) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_INVALID_ARG;
    }

    char nvs_key[MAX_NVS_KEY_LENGTH + 1];
    create_nvs_key(id, nvs_key);

    esp_err_t err = nvs_get_i32(m_nvs_handle, nvs_key, value);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // Use default value if available
        if (it->second.default_value != nullptr) {
            *value = *static_cast<int32_t*>(it->second.default_value);
            err = ESP_OK;
        }
    }

    xSemaphoreGive(m_mutex);
    return err;
}

esp_err_t ParameterManager::get_float(const char* id, float* value)
{
    if (!validate_id(id) || value == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    if (m_nvs_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_mutex, portMAX_DELAY);

    auto it = m_parameters.find(id);
    if (it == m_parameters.end()) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    if (it->second.type != ParameterType::Float) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_INVALID_ARG;
    }

    char nvs_key[MAX_NVS_KEY_LENGTH + 1];
    create_nvs_key(id, nvs_key);

    // NVS doesn't have native float support, store as blob
    size_t required_size = sizeof(float);
    esp_err_t err = nvs_get_blob(m_nvs_handle, nvs_key, value, &required_size);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // Use default value if available
        if (it->second.default_value != nullptr) {
            *value = *static_cast<float*>(it->second.default_value);
            err = ESP_OK;
        }
    }

    xSemaphoreGive(m_mutex);
    return err;
}

esp_err_t ParameterManager::get_bool(const char* id, bool* value)
{
    if (!validate_id(id) || value == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    if (m_nvs_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_mutex, portMAX_DELAY);

    auto it = m_parameters.find(id);
    if (it == m_parameters.end()) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    if (it->second.type != ParameterType::Boolean) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_INVALID_ARG;
    }

    char nvs_key[MAX_NVS_KEY_LENGTH + 1];
    create_nvs_key(id, nvs_key);

    uint8_t bool_val;
    esp_err_t err = nvs_get_u8(m_nvs_handle, nvs_key, &bool_val);

    if (err == ESP_OK) {
        *value = (bool_val != 0);
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        // Use default value if available
        if (it->second.default_value != nullptr) {
            *value = *static_cast<bool*>(it->second.default_value);
            err = ESP_OK;
        }
    }

    xSemaphoreGive(m_mutex);
    return err;
}

esp_err_t ParameterManager::set_string(const char* id, const char* value, bool trigger_callback)
{
    if (!validate_id(id) || value == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    if (m_nvs_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_mutex, portMAX_DELAY);

    auto it = m_parameters.find(id);
    if (it == m_parameters.end()) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    if (it->second.type != ParameterType::String) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_INVALID_ARG;
    }

    char nvs_key[MAX_NVS_KEY_LENGTH + 1];
    create_nvs_key(id, nvs_key);

    esp_err_t err = nvs_set_str(m_nvs_handle, nvs_key, value);
    if (err == ESP_OK) {
        err = nvs_commit(m_nvs_handle);
    }

    xSemaphoreGive(m_mutex);

    if (err == ESP_OK && trigger_callback) {
        this->trigger_callback(id);
    }

    return err;
}

esp_err_t ParameterManager::set_int(const char* id, int32_t value, bool trigger_callback)
{
    if (!validate_id(id)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (m_nvs_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_mutex, portMAX_DELAY);

    auto it = m_parameters.find(id);
    if (it == m_parameters.end()) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    if (it->second.type != ParameterType::Integer) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_INVALID_ARG;
    }

    char nvs_key[MAX_NVS_KEY_LENGTH + 1];
    create_nvs_key(id, nvs_key);

    esp_err_t err = nvs_set_i32(m_nvs_handle, nvs_key, value);
    if (err == ESP_OK) {
        err = nvs_commit(m_nvs_handle);
    }

    xSemaphoreGive(m_mutex);

    if (err == ESP_OK && trigger_callback) {
        this->trigger_callback(id);
    }

    return err;
}

esp_err_t ParameterManager::set_float(const char* id, float value, bool trigger_callback)
{
    if (!validate_id(id)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (m_nvs_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_mutex, portMAX_DELAY);

    auto it = m_parameters.find(id);
    if (it == m_parameters.end()) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    if (it->second.type != ParameterType::Float) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_INVALID_ARG;
    }

    char nvs_key[MAX_NVS_KEY_LENGTH + 1];
    create_nvs_key(id, nvs_key);

    // Store float as blob
    esp_err_t err = nvs_set_blob(m_nvs_handle, nvs_key, &value, sizeof(float));
    if (err == ESP_OK) {
        err = nvs_commit(m_nvs_handle);
    }

    xSemaphoreGive(m_mutex);

    if (err == ESP_OK && trigger_callback) {
        this->trigger_callback(id);
    }

    return err;
}

esp_err_t ParameterManager::set_bool(const char* id, bool value, bool trigger_callback)
{
    if (!validate_id(id)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (m_nvs_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_mutex, portMAX_DELAY);

    auto it = m_parameters.find(id);
    if (it == m_parameters.end()) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    if (it->second.type != ParameterType::Boolean) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_INVALID_ARG;
    }

    char nvs_key[MAX_NVS_KEY_LENGTH + 1];
    create_nvs_key(id, nvs_key);

    uint8_t bool_val = value ? 1 : 0;
    esp_err_t err = nvs_set_u8(m_nvs_handle, nvs_key, bool_val);
    if (err == ESP_OK) {
        err = nvs_commit(m_nvs_handle);
    }

    xSemaphoreGive(m_mutex);

    if (err == ESP_OK && trigger_callback) {
        this->trigger_callback(id);
    }

    return err;
}

esp_err_t ParameterManager::register_callback(const char* id, parameter_change_callback_t callback, void* user_data)
{
    if (!validate_id(id) || callback == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_mutex, portMAX_DELAY);

    auto it = m_parameters.find(id);
    if (it == m_parameters.end()) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    // Check if callback is already registered
    for (const auto& entry : it->second.callbacks) {
        if (entry.callback == callback) {
            xSemaphoreGive(m_mutex);
            ESP_LOGW(TAG, "Callback already registered for parameter: %s", id);
            return ESP_OK;  // Already registered, not an error
        }
    }

    // Add new callback entry
    CallbackEntry entry;
    entry.callback = callback;
    entry.user_data = user_data;
    it->second.callbacks.push_back(entry);

    xSemaphoreGive(m_mutex);

    #ifdef CONFIG_PARAMETER_MANAGER_ENABLE_LOGGING
    ESP_LOGI(TAG, "Registered callback for parameter: %s (total callbacks: %d)",
             id, it->second.callbacks.size());
    #endif
    return ESP_OK;
}

esp_err_t ParameterManager::unregister_callback(const char* id, parameter_change_callback_t callback)
{
    if (!validate_id(id)) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_mutex, portMAX_DELAY);

    auto it = m_parameters.find(id);
    if (it == m_parameters.end()) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    if (callback == nullptr) {
        // Remove all callbacks
        if (it->second.callbacks.empty()) {
            xSemaphoreGive(m_mutex);
            return ESP_ERR_NOT_FOUND;
        }
        it->second.callbacks.clear();
        xSemaphoreGive(m_mutex);
        #ifdef CONFIG_PARAMETER_MANAGER_ENABLE_LOGGING
        ESP_LOGI(TAG, "Unregistered all callbacks for parameter: %s", id);
        #endif
        return ESP_OK;
    }

    // Remove specific callback
    auto& callbacks = it->second.callbacks;
    for (auto cb_it = callbacks.begin(); cb_it != callbacks.end(); ++cb_it) {
        if (cb_it->callback == callback) {
            callbacks.erase(cb_it);
            xSemaphoreGive(m_mutex);
            #ifdef CONFIG_PARAMETER_MANAGER_ENABLE_LOGGING
            ESP_LOGI(TAG, "Unregistered callback for parameter: %s (remaining: %d)",
                     id, callbacks.size());
            #endif
            return ESP_OK;
        }
    }

    xSemaphoreGive(m_mutex);
    return ESP_ERR_NOT_FOUND;
}

void ParameterManager::trigger_callback(const char* id)
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);

    auto it = m_parameters.find(id);
    if (it == m_parameters.end() || it->second.callbacks.empty()) {
        xSemaphoreGive(m_mutex);
        return;
    }

    // Copy callback list to avoid holding mutex during callback execution
    std::vector<CallbackEntry> callbacks_copy = it->second.callbacks;

    // Release mutex before calling callbacks to avoid deadlocks
    xSemaphoreGive(m_mutex);

    // Call all registered callbacks
    for (const auto& entry : callbacks_copy) {
        entry.callback(id, entry.user_data);
    }
}

esp_err_t ParameterManager::reset_to_default(const char* id)
{
    if (!validate_id(id)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (m_nvs_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_mutex, portMAX_DELAY);

    auto it = m_parameters.find(id);
    if (it == m_parameters.end()) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    // Erase from NVS (will use default value on next get)
    char nvs_key[MAX_NVS_KEY_LENGTH + 1];
    create_nvs_key(id, nvs_key);
    nvs_erase_key(m_nvs_handle, nvs_key);
    esp_err_t err = nvs_commit(m_nvs_handle);

    xSemaphoreGive(m_mutex);

    if (err == ESP_OK) {
        trigger_callback(id);
    }

    return err;
}

esp_err_t ParameterManager::reset_all_to_defaults()
{
    if (m_nvs_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_mutex, portMAX_DELAY);

    // Erase all keys from NVS namespace
    nvs_erase_all(m_nvs_handle);
    esp_err_t err = nvs_commit(m_nvs_handle);

    // Trigger callbacks for all parameters
    std::vector<std::string> param_ids;
    for (const auto& pair : m_parameters) {
        param_ids.push_back(pair.first);
    }

    xSemaphoreGive(m_mutex);

    // Trigger callbacks outside of mutex
    for (const auto& id : param_ids) {
        trigger_callback(id.c_str());
    }

    #ifdef CONFIG_PARAMETER_MANAGER_ENABLE_LOGGING
    ESP_LOGI(TAG, "Reset all parameters to defaults");
    #endif
    return err;
}

esp_err_t ParameterManager::list_parameters(char ids[][33], size_t max_count, size_t* count)
{
    if (ids == nullptr || count == nullptr || max_count == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_mutex, portMAX_DELAY);

    size_t idx = 0;
    for (const auto& pair : m_parameters) {
        if (idx >= max_count) {
            break;
        }
        strncpy(ids[idx], pair.first.c_str(), 32);
        ids[idx][32] = '\0';
        idx++;
    }

    *count = idx;

    xSemaphoreGive(m_mutex);

    return ESP_OK;
}

} // namespace macdap
