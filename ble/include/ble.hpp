/**
 * @file ble.hpp
 * @brief BLE GATT service for parameter management
 *
 * This component provides a Bluetooth Low Energy GATT service for managing
 * device parameters via mobile apps. It mirrors the REST API functionality
 * but is optimized for BLE operation.
 *
 * Service UUID: 12340000-1234-1234-1234-123456789abc
 *
 * Characteristics:
 * - Parameter List (12340001-...)     - Read, Notify
 * - Parameter Access (12340002-...)   - Read, Write
 * - Parameter Delete (12340003-...)   - Write
 * - Parameter Notify (12340004-...)   - Notify
 * - Control (12340005-...)            - Write
 */

#pragma once

#include <esp_err.h>
#include <esp_bt.h>
#include <esp_gap_ble_api.h>
#include <esp_gatts_api.h>
#include <esp_bt_main.h>
#include <esp_gatt_common_api.h>
#include <parameterManager.hpp>

namespace macdap {

// Service and Characteristic UUIDs
#define BLE_PARAM_SERVICE_UUID { 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x34, 0x12, 0x34, 0x12, 0x34, 0x12, 0x00, 0x00, 0x34, 0x12 }
#define BLE_PARAM_LIST_CHAR_UUID { 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x34, 0x12, 0x34, 0x12, 0x34, 0x12, 0x01, 0x00, 0x34, 0x12 }
#define BLE_PARAM_ACCESS_CHAR_UUID { 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x34, 0x12, 0x34, 0x12, 0x34, 0x12, 0x02, 0x00, 0x34, 0x12 }
#define BLE_PARAM_DELETE_CHAR_UUID { 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x34, 0x12, 0x34, 0x12, 0x34, 0x12, 0x03, 0x00, 0x34, 0x12 }
#define BLE_PARAM_NOTIFY_CHAR_UUID { 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x34, 0x12, 0x34, 0x12, 0x34, 0x12, 0x04, 0x00, 0x34, 0x12 }
#define BLE_PARAM_CONTROL_CHAR_UUID { 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x34, 0x12, 0x34, 0x12, 0x34, 0x12, 0x05, 0x00, 0x34, 0x12 }

// Configuration
#define BLE_PARAM_SERVICE_MTU 512
#define BLE_PARAM_MAX_PARAM_NAME_LEN 33
#define BLE_PARAM_MAX_STRING_VALUE_LEN 256

/**
 * @brief BLE Parameter Service class
 *
 * Provides BLE GATT service for parameter management using the ParameterManager backend.
 * Implements singleton pattern for global access.
 */
class BLE {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to the BLE instance
     */
    static BLE& get_instance();

    /**
     * @brief Initialize BLE stack and service
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t init();

    /**
     * @brief Start BLE advertising
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t start();

    /**
     * @brief Stop BLE advertising
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t stop();

    /**
     * @brief Check if service is running
     * @return true if running, false otherwise
     */
    bool is_running() const;

    /**
     * @brief Enable or disable parameter change notifications
     * @param enable true to enable, false to disable
     */
    void enable_notifications(bool enable);

    // Delete copy constructor and assignment
    BLE(BLE const&) = delete;
    void operator=(BLE const&) = delete;

private:
    BLE();
    ~BLE();

    // BLE event handlers
    static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
    static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

    // Instance event handlers
    void handle_gatts_register_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
    void handle_gatts_read_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
    void handle_gatts_write_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
    void handle_gatts_connect_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
    void handle_gatts_disconnect_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

    // Characteristic handlers
    esp_err_t handle_list_read(esp_ble_gatts_cb_param_t *param);
    esp_err_t handle_access_read(esp_ble_gatts_cb_param_t *param);
    esp_gatt_status_t handle_access_write(const uint8_t *data, uint16_t len);
    esp_gatt_status_t handle_delete_write(const uint8_t *data, uint16_t len);
    esp_gatt_status_t handle_control_write(const uint8_t *data, uint16_t len);

    // Parameter change callback
    static void parameter_change_callback(const char* id, void* user_data);

    // Notification helpers
    void send_parameter_notification(const char* param_name);
    void send_notification(uint16_t char_handle, const uint8_t* data, uint16_t len);

    // JSON encoding/decoding
    char* encode_parameter_list();
    char* encode_parameter(const char* name);
    esp_err_t decode_set_request(const char* json, char* name, size_t name_len);
    esp_err_t decode_delete_request(const char* json, char* name, size_t name_len);
    esp_err_t decode_control_request(const char* json, char* command, size_t command_len);

    // Service registration
    esp_err_t create_service();
    esp_err_t add_characteristics();
    esp_err_t start_service();

    // State
    bool m_initialized;
    bool m_running;
    bool m_notifications_enabled;
    bool m_connected;
    uint16_t m_conn_id;
    esp_gatt_if_t m_gatts_if;
    uint16_t m_service_handle;

    // Advertising data
    esp_ble_adv_data_t m_adv_data;
    esp_ble_adv_params_t m_adv_params;

    // Characteristic handles
    uint16_t m_char_list_handle;
    uint16_t m_char_list_val_handle;
    uint16_t m_char_access_handle;
    uint16_t m_char_access_val_handle;
    uint16_t m_char_delete_handle;
    uint16_t m_char_delete_val_handle;
    uint16_t m_char_notify_handle;
    uint16_t m_char_notify_val_handle;
    uint16_t m_char_control_handle;
    uint16_t m_char_control_val_handle;

    // Descriptor handles (for CCCD)
    uint16_t m_char_list_cccd_handle;
    uint16_t m_char_notify_cccd_handle;

    // Temporary storage for parameter access (write param name, then read value)
    char m_current_param_name[BLE_PARAM_MAX_PARAM_NAME_LEN];

    // Singleton instance pointer for static callbacks
    static BLE* s_instance;
};

} // namespace macdap
