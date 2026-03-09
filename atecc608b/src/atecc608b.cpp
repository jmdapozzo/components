#include "atecc608b.hpp"
#include <esp_log.h>
#include <esp_err.h>
#include <esp_check.h>
#include <esp_crc.h>
#include <string.h>
#include <freertos/task.h>

// cryptoauthlib
#include "cryptoauthlib.h"
#include "atcacert/atcacert_client.h"
#include "mbedtls/atca_mbedtls_wrap.h"

// ESP-IDF I2C driver — needed to release the bus created by i2c_manager
#include <driver/i2c_master.h>

using namespace macdap;

static const char *TAG = "atecc608b";

// ── Helper: CRC32 over the first 60 bytes of ManufacturingData ───────────────

static uint32_t otp_crc32(const ManufacturingData* d)
{
    return esp_crc32_le(0, reinterpret_cast<const uint8_t*>(d),
                        offsetof(ManufacturingData, crc32));
}

// ── Helper: restore I2C bus after cryptoauthlib tears it down ─────────────────
//
// cryptoauthlib's HAL (hal_i2c_release) calls i2c_del_master_bus() when the
// reference count drops to zero (i.e. after atcab_release() or a failed init).
// All other components in this project use i2c_master_get_bus_handle() to
// retrieve the bus. If the bus is gone they abort. This function recreates the
// bus so subsequent components can find it again.
//
// The returned handle is intentionally not deleted — the bus must stay live for
// the duration of the application, matching how i2c_manager creates its bus.

static void restore_i2c_bus(int port)
{
    i2c_master_bus_handle_t existing;
    if (i2c_master_get_bus_handle(port, &existing) == ESP_OK) {
        return;  // bus still alive — nothing to do
    }

    i2c_master_bus_config_t config = {};
    config.i2c_port                    = port;
    config.sda_io_num                  = static_cast<gpio_num_t>(CONFIG_ATCA_I2C_SDA_PIN);
    config.scl_io_num                  = static_cast<gpio_num_t>(CONFIG_ATCA_I2C_SCL_PIN);
    config.clk_source                  = I2C_CLK_SRC_DEFAULT;
    config.glitch_ignore_cnt           = 7;
    config.flags.enable_internal_pullup = true;

    i2c_master_bus_handle_t handle;
    if (i2c_new_master_bus(&config, &handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to restore I2C bus on port %d after chip detection", port);
    }
}

// ── Constructor ───────────────────────────────────────────────────────────────

SecureElement::SecureElement()
    : m_is_present(false), m_config_locked(false), m_data_locked(false),
      m_semaphore_handle(nullptr)
{
    ESP_LOGI(TAG, "Initializing ATECC608B...");

    // Build cryptoauthlib I2C config.
    // atcai2c.address is the 8-bit write address (7-bit << 1).
    m_config = cfg_ateccx08a_i2c_default;
    m_config.atcai2c.address  = static_cast<uint8_t>(CONFIG_ATECC608B_I2C_ADDRESS << 1);
    m_config.atcai2c.bus      = CONFIG_ATECC608B_I2C_PORT_NUM;
    m_config.atcai2c.baud     = CONFIG_ATECC608B_I2C_SPEED_HZ;

    ATCA_STATUS status = atcab_init(&m_config);
    if (status != ATCA_SUCCESS)
    {
        restore_i2c_bus(CONFIG_ATECC608B_I2C_PORT_NUM);
        ESP_LOGW(TAG, "atcab_init failed (0x%02x) — chip not found or I2C error", status);
        return;
    }

    // Verify communication by reading the chip revision
    uint8_t revision[4];
    status = atcab_info(revision);
    if (status != ATCA_SUCCESS)
    {
        atcab_release();
        restore_i2c_bus(CONFIG_ATECC608B_I2C_PORT_NUM);
        ESP_LOGW(TAG, "atcab_info failed (0x%02x) — chip not responding", status);
        return;
    }

    ESP_LOGI(TAG, "ATECC608B found, revision: %02X %02X %02X %02X",
             revision[0], revision[1], revision[2], revision[3]);

    m_is_present = true;

    m_semaphore_handle = xSemaphoreCreateMutex();
    if (m_semaphore_handle == nullptr)
    {
        ESP_LOGE(TAG, "Failed to create semaphore");
        m_is_present = false;
        atcab_release();
        restore_i2c_bus(CONFIG_ATECC608B_I2C_PORT_NUM);
        return;
    }

    // Read and cache lock status
    if (check_lock_status() != ESP_OK)
    {
        ESP_LOGW(TAG, "Could not read lock status");
    }

    ESP_LOGI(TAG, "Config zone: %s | Data zone: %s",
             m_config_locked ? "LOCKED" : "unlocked",
             m_data_locked   ? "LOCKED" : "unlocked");


    ESP_LOGI(TAG, "Initialization complete");
}

SecureElement::~SecureElement()
{
    if (m_semaphore_handle != nullptr)
    {
        vSemaphoreDelete(m_semaphore_handle);
    }
    if (m_is_present)
    {
        atcab_release();
    }
}

// ── Private helpers ───────────────────────────────────────────────────────────

esp_err_t SecureElement::check_lock_status()
{
    bool locked = false;

    ATCA_STATUS status = atcab_is_locked(LOCK_ZONE_CONFIG, &locked);
    if (status != ATCA_SUCCESS)
    {
        return ESP_FAIL;
    }
    m_config_locked = locked;

    status = atcab_is_locked(LOCK_ZONE_DATA, &locked);
    if (status != ATCA_SUCCESS)
    {
        return ESP_FAIL;
    }
    m_data_locked = locked;

    return ESP_OK;
}

// ── Chip identity ─────────────────────────────────────────────────────────────

esp_err_t SecureElement::get_chip_serial(uint8_t serial[ATECC608B_CHIP_SERIAL_SIZE])
{
    if (!m_is_present || serial == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    ATCA_STATUS status = atcab_read_serial_number(serial);
    xSemaphoreGive(m_semaphore_handle);

    if (status != ATCA_SUCCESS)
    {
        ESP_LOGE(TAG, "atcab_read_serial_number failed (0x%02x)", status);
        return ESP_FAIL;
    }
    return ESP_OK;
}

// ── OTP zone ──────────────────────────────────────────────────────────────────

esp_err_t SecureElement::read_otp(uint8_t data[ATECC608B_OTP_SIZE])
{
    if (!m_is_present || data == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    ATCA_STATUS status = atcab_read_bytes_zone(ATCA_ZONE_OTP, 0, 0,
                                               data, ATECC608B_OTP_SIZE);
    xSemaphoreGive(m_semaphore_handle);

    if (status != ATCA_SUCCESS)
    {
        ESP_LOGE(TAG, "atcab_read_bytes_zone(OTP) failed (0x%02x)", status);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t SecureElement::write_otp(const uint8_t data[ATECC608B_OTP_SIZE])
{
    if (!m_is_present || data == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (m_data_locked)
    {
        ESP_LOGE(TAG, "Cannot write OTP zone — data zone is locked");
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    ATCA_STATUS status = atcab_write_bytes_zone(ATCA_ZONE_OTP, 0, 0,
                                                data, ATECC608B_OTP_SIZE);
    xSemaphoreGive(m_semaphore_handle);

    if (status != ATCA_SUCCESS)
    {
        ESP_LOGE(TAG, "atcab_write_bytes_zone(OTP) failed (0x%02x)", status);
        return ESP_FAIL;
    }
    return ESP_OK;
}

// ── Manufacturing data ────────────────────────────────────────────────────────

esp_err_t SecureElement::read_manufacturing_data(ManufacturingData* data)
{
    if (data == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t raw[ATECC608B_OTP_SIZE];
    esp_err_t err = read_otp(raw);
    if (err != ESP_OK)
    {
        return err;
    }

    memcpy(data, raw, ATECC608B_OTP_SIZE);

    if (data->magic != CONFIG_ATECC608B_OTP_MAGIC)
    {
        ESP_LOGW(TAG, "OTP magic mismatch: expected 0x%08lX, got 0x%08lX",
                 (unsigned long)CONFIG_ATECC608B_OTP_MAGIC,
                 (unsigned long)data->magic);
        return ESP_ERR_NOT_FOUND;
    }

    uint32_t expected_crc = otp_crc32(data);
    if (data->crc32 != expected_crc)
    {
        ESP_LOGE(TAG, "OTP CRC32 mismatch: expected 0x%08lX, got 0x%08lX",
                 (unsigned long)expected_crc,
                 (unsigned long)data->crc32);
        return ESP_ERR_INVALID_CRC;
    }

    return ESP_OK;
}

esp_err_t SecureElement::write_manufacturing_data(const ManufacturingData* data)
{
    if (data == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Build a mutable copy, fill in magic and CRC
    ManufacturingData d;
    memcpy(&d, data, sizeof(d));
    d.magic = CONFIG_ATECC608B_OTP_MAGIC;
    d.crc32 = otp_crc32(&d);

    uint8_t raw[ATECC608B_OTP_SIZE];
    memcpy(raw, &d, ATECC608B_OTP_SIZE);

    esp_err_t err = write_otp(raw);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Manufacturing data written to OTP zone");
        ESP_LOGI(TAG, "  Serial:       %s", d.serial);
        ESP_LOGI(TAG, "  Manufacturer: %s", d.manufacturer);
        ESP_LOGI(TAG, "  Model:        %s", d.model);
        ESP_LOGI(TAG, "  HW revision:  %d.%d", d.hw_rev_major, d.hw_rev_minor);
        ESP_LOGI(TAG, "  Mfg date:     %08lu", (unsigned long)d.mfg_date);
    }
    return err;
}

// ── Certificates ──────────────────────────────────────────────────────────────

esp_err_t SecureElement::read_cert(uint8_t slot, uint8_t* der, size_t* len)
{
    if (!m_is_present || der == nullptr || len == nullptr || *len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    // Only slot 8 can hold up to 416 bytes; slots 9-15 are 72 bytes each
    ATCA_STATUS status = atcab_read_bytes_zone(ATCA_ZONE_DATA, slot, 0, der,
                                               static_cast<uint16_t>(*len));
    xSemaphoreGive(m_semaphore_handle);

    if (status != ATCA_SUCCESS)
    {
        ESP_LOGE(TAG, "read_cert slot %d failed (0x%02x)", slot, status);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t SecureElement::write_cert(uint8_t slot, const uint8_t* der, size_t len)
{
    if (!m_is_present || der == nullptr || len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (m_data_locked)
    {
        ESP_LOGE(TAG, "Cannot write cert — data zone is locked");
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    ATCA_STATUS status = atcab_write_bytes_zone(ATCA_ZONE_DATA, slot, 0, der,
                                                static_cast<uint16_t>(len));
    xSemaphoreGive(m_semaphore_handle);

    if (status != ATCA_SUCCESS)
    {
        ESP_LOGE(TAG, "write_cert slot %d failed (0x%02x)", slot, status);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Certificate (%d bytes) written to slot %d", (int)len, slot);
    return ESP_OK;
}

// ── Key operations ────────────────────────────────────────────────────────────

esp_err_t SecureElement::gen_key(uint8_t slot, uint8_t public_key[ATECC608B_PUBLIC_KEY_SIZE])
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (m_data_locked)
    {
        // GenKey is allowed after lock if slot policy permits
        ESP_LOGW(TAG, "gen_key called after data zone lock — slot policy must allow GenKey");
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    ATCA_STATUS status = atcab_genkey(slot, public_key);
    xSemaphoreGive(m_semaphore_handle);

    if (status != ATCA_SUCCESS)
    {
        ESP_LOGE(TAG, "atcab_genkey slot %d failed (0x%02x)", slot, status);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "ECC key generated in slot %d (private key stays in chip)", slot);
    return ESP_OK;
}

esp_err_t SecureElement::sign(uint8_t slot,
                          const uint8_t digest[ATECC608B_DIGEST_SIZE],
                          uint8_t signature[ATECC608B_SIGNATURE_SIZE])
{
    if (!m_is_present || digest == nullptr || signature == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    ATCA_STATUS status = atcab_sign(slot, digest, signature);
    xSemaphoreGive(m_semaphore_handle);

    if (status != ATCA_SUCCESS)
    {
        ESP_LOGE(TAG, "atcab_sign slot %d failed (0x%02x)", slot, status);
        return ESP_FAIL;
    }
    return ESP_OK;
}

// ── Zone locking ──────────────────────────────────────────────────────────────

esp_err_t SecureElement::lock_config_zone()
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (m_config_locked)
    {
        ESP_LOGW(TAG, "Config zone is already locked");
        return ESP_OK;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    ATCA_STATUS status = atcab_lock_config_zone();
    xSemaphoreGive(m_semaphore_handle);

    if (status != ATCA_SUCCESS)
    {
        ESP_LOGE(TAG, "atcab_lock_config_zone failed (0x%02x)", status);
        return ESP_FAIL;
    }

    m_config_locked = true;
    ESP_LOGW(TAG, "Config zone LOCKED — this is permanent");
    return ESP_OK;
}

esp_err_t SecureElement::lock_data_zone()
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (!m_config_locked)
    {
        ESP_LOGE(TAG, "Config zone must be locked before data zone");
        return ESP_ERR_INVALID_STATE;
    }

    if (m_data_locked)
    {
        ESP_LOGW(TAG, "Data zone is already locked");
        return ESP_OK;
    }

    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    ATCA_STATUS status = atcab_lock_data_zone();
    xSemaphoreGive(m_semaphore_handle);

    if (status != ATCA_SUCCESS)
    {
        ESP_LOGE(TAG, "atcab_lock_data_zone failed (0x%02x)", status);
        return ESP_FAIL;
    }

    m_data_locked = true;
    ESP_LOGW(TAG, "Data zone LOCKED — this is permanent");
    return ESP_OK;
}

// ── TLS integration ───────────────────────────────────────────────────────────

esp_err_t SecureElement::init_tls_alt_key()
{
    if (!m_is_present)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (!m_data_locked)
    {
        ESP_LOGW(TAG, "Data zone not locked — TLS requires a provisioned chip");
        return ESP_ERR_INVALID_STATE;
    }

    // Verify that slot 0 holds an ECC key by reading its corresponding public key.
    // atcab_get_pubkey() returns the public key associated with the private key in
    // the given slot. If no key has been generated, it returns an error.
    uint8_t pub_key[ATECC608B_PUBLIC_KEY_SIZE];
    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    ATCA_STATUS status = atcab_get_pubkey(ATECC608B_SLOT_PRIVATE_KEY, pub_key);
    xSemaphoreGive(m_semaphore_handle);

    if (status != ATCA_SUCCESS)
    {
        ESP_LOGE(TAG, "atcab_get_pubkey slot %d failed (0x%02x) — no key generated?",
                 ATECC608B_SLOT_PRIVATE_KEY, status);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "TLS key verified in slot %d — use init_tls_pk_context() per connection",
             ATECC608B_SLOT_PRIVATE_KEY);
    return ESP_OK;
}

esp_err_t SecureElement::init_tls_pk_context(mbedtls_pk_context* ctx)
{
    if (!m_is_present || ctx == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!m_data_locked)
    {
        ESP_LOGE(TAG, "Data zone not locked — TLS pk context requires a provisioned chip");
        return ESP_ERR_INVALID_STATE;
    }

    // atca_mbedtls_pk_init() binds the hardware private key in the given slot to
    // an mbedTLS pk_context. All subsequent ECDSA operations on this context are
    // performed inside the ATECC608B — the private key never leaves the chip.
    // The caller must call mbedtls_pk_free(ctx) when done with the TLS connection.
    int ret = atca_mbedtls_pk_init(ctx, ATECC608B_SLOT_PRIVATE_KEY);
    if (ret != 0)
    {
        ESP_LOGE(TAG, "atca_mbedtls_pk_init failed (%d)", ret);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "mbedTLS pk_context initialized with hardware key (slot %d)",
             ATECC608B_SLOT_PRIVATE_KEY);
    return ESP_OK;
}
