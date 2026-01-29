/**
 * @file parameterManager.hpp
 * @brief Parameter management with NVS storage and change callbacks
 *
 * This component provides a centralized parameter management system with:
 * - NVS-backed persistent storage
 * - Thread-safe access
 * - Type-safe parameter operations
 * - Change notification callbacks
 * - Support for multiple parameter types (String, Integer, Float, Boolean)
 */

#pragma once

#include <esp_err.h>
#include <nvs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <map>
#include <string>
#include <vector>

namespace macdap {

/**
 * @brief Parameter management class
 *
 * Singleton class that manages parameters with NVS storage.
 * Parameters are identified by a unique ID (max 32 characters) and
 * support callbacks for change notifications.
 */
class ParameterManager {
public:
    /**
     * @brief Supported parameter types
     */
    enum class ParameterType {
        String,     ///< String parameter (null-terminated)
        Integer,    ///< 32-bit signed integer
        Float,      ///< 32-bit floating point
        Boolean     ///< Boolean (true/false)
    };

    /**
     * @brief Callback function type for parameter changes
     * @param id Parameter ID that changed
     * @param user_data User data passed during callback registration
     */
    typedef void (*parameter_change_callback_t)(const char* id, void* user_data);

    /**
     * @brief Get the singleton instance
     * @return Reference to the ParameterManager instance
     */
    static ParameterManager& get_instance();

    /**
     * @brief Create a new parameter or update existing default value
     * @param id Parameter ID (max 32 characters, must be unique)
     * @param type Parameter type
     * @param default_value Optional default value (type-specific pointer)
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t create_parameter(const char* id, ParameterType type, const void* default_value = nullptr);

    /**
     * @brief Delete a parameter and its stored value
     * @param id Parameter ID
     * @return ESP_OK on success, ESP_ERR_NOT_FOUND if parameter doesn't exist
     */
    esp_err_t delete_parameter(const char* id);

    /**
     * @brief Check if a parameter exists
     * @param id Parameter ID
     * @return true if parameter exists, false otherwise
     */
    bool exists(const char* id);

    /**
     * @brief Get parameter type
     * @param id Parameter ID
     * @param type Output parameter type
     * @return ESP_OK on success, ESP_ERR_NOT_FOUND if parameter doesn't exist
     */
    esp_err_t get_type(const char* id, ParameterType* type);

    /**
     * @brief Get string parameter value
     * @param id Parameter ID
     * @param value Buffer to store the string value
     * @param max_len Maximum buffer length
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t get_string(const char* id, char* value, size_t max_len);

    /**
     * @brief Get integer parameter value
     * @param id Parameter ID
     * @param value Pointer to store the integer value
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t get_int(const char* id, int32_t* value);

    /**
     * @brief Get float parameter value
     * @param id Parameter ID
     * @param value Pointer to store the float value
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t get_float(const char* id, float* value);

    /**
     * @brief Get boolean parameter value
     * @param id Parameter ID
     * @param value Pointer to store the boolean value
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t get_bool(const char* id, bool* value);

    /**
     * @brief Set string parameter value
     * @param id Parameter ID
     * @param value String value to set
     * @param trigger_callback Whether to trigger change callback (default: true)
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t set_string(const char* id, const char* value, bool trigger_callback = true);

    /**
     * @brief Set integer parameter value
     * @param id Parameter ID
     * @param value Integer value to set
     * @param trigger_callback Whether to trigger change callback (default: true)
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t set_int(const char* id, int32_t value, bool trigger_callback = true);

    /**
     * @brief Set float parameter value
     * @param id Parameter ID
     * @param value Float value to set
     * @param trigger_callback Whether to trigger change callback (default: true)
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t set_float(const char* id, float value, bool trigger_callback = true);

    /**
     * @brief Set boolean parameter value
     * @param id Parameter ID
     * @param value Boolean value to set
     * @param trigger_callback Whether to trigger change callback (default: true)
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t set_bool(const char* id, bool value, bool trigger_callback = true);

    /**
     * @brief Register a callback for parameter changes
     * @param id Parameter ID to monitor
     * @param callback Callback function to call when parameter changes
     * @param user_data Optional user data passed to callback
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t register_callback(const char* id, parameter_change_callback_t callback, void* user_data = nullptr);

    /**
     * @brief Unregister a callback for parameter changes
     * @param id Parameter ID
     * @param callback Callback function to unregister (nullptr = unregister all)
     * @return ESP_OK on success, ESP_ERR_NOT_FOUND if callback not found
     */
    esp_err_t unregister_callback(const char* id, parameter_change_callback_t callback = nullptr);

    /**
     * @brief Reset parameter to its default value
     * @param id Parameter ID
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t reset_to_default(const char* id);

    /**
     * @brief Reset all parameters to their default values
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t reset_all_to_defaults();

    /**
     * @brief Get list of all parameter IDs
     * @param ids Buffer to store parameter IDs
     * @param max_count Maximum number of IDs to return
     * @param count Output actual number of IDs returned
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t list_parameters(char ids[][33], size_t max_count, size_t* count);

    // Delete copy constructor and assignment operator
    ParameterManager(ParameterManager const&) = delete;
    void operator=(ParameterManager const&) = delete;

private:
    ParameterManager();
    ~ParameterManager();

    /**
     * @brief Callback entry structure
     */
    struct CallbackEntry {
        parameter_change_callback_t callback;
        void* user_data;
    };

    /**
     * @brief Internal parameter metadata structure
     */
    struct ParameterInfo {
        ParameterType type;
        void* default_value;                    ///< Pointer to default value (type-specific)
        std::vector<CallbackEntry> callbacks;   ///< List of registered callbacks
    };

    SemaphoreHandle_t m_mutex;                      ///< Mutex for thread-safe access
    std::map<std::string, ParameterInfo> m_parameters;  ///< Parameter registry
    nvs_handle_t m_nvs_handle;                      ///< NVS handle for storage

    /**
     * @brief Validate parameter ID
     * @param id Parameter ID to validate
     * @return true if valid, false otherwise
     */
    bool validate_id(const char* id);

    /**
     * @brief Create NVS key from parameter ID
     * @param id Parameter ID
     * @param key Output NVS key (max 15 chars for NVS)
     */
    void create_nvs_key(const char* id, char* key);

    /**
     * @brief Trigger callback if registered for parameter
     * @param id Parameter ID
     */
    void trigger_callback(const char* id);
};

} // namespace macdap
