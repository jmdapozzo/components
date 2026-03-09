#include "crypto_manager.h"

#include <string.h>
#include <stdio.h>
#include <esp_log.h>

#include <atecc608b.hpp>

static const char *TAG = "crypto_manager";

// ── Cache variables (zero-initialised at startup) ─────────────────────────────

char crypto_device_serial[16]       = {};
char crypto_device_manufacturer[16] = {};
char crypto_device_model[16]        = {};
char crypto_hw_revision[8]          = {};
char crypto_chip_serial_hex[19]     = {};

// ── Module state ──────────────────────────────────────────────────────────────

static bool s_chip_present    = false;
static bool s_provisioned     = false;
static bool s_data_locked     = false;

// ── Public API ────────────────────────────────────────────────────────────────

bool crypto_manager_chip_present(void)  { return s_chip_present; }
bool crypto_manager_is_provisioned(void){ return s_provisioned;   }
bool crypto_manager_data_locked(void)   { return s_data_locked;   }

void init_crypto_manager(void)
{
    ESP_LOGI(TAG, "Initializing crypto manager...");

    macdap::SecureElement& chip = macdap::SecureElement::get_instance();

    if (!chip.is_present())
    {
        ESP_LOGW(TAG, "ATECC608B not detected — running without secure element");
        return;
    }

    s_chip_present = true;
    s_data_locked  = chip.is_data_locked();

    // ── Read chip serial number (always available) ────────────────────────
    uint8_t chip_serial[macdap::ATECC608B_CHIP_SERIAL_SIZE] = {};
    if (chip.get_chip_serial(chip_serial) == ESP_OK)
    {
        // Format as hex string: "XX:XX:XX:XX:XX:XX:XX:XX:XX" (26 chars with colons)
        // We use compact hex without colons to fit in 19 chars: 18 hex + null
        snprintf(crypto_chip_serial_hex, sizeof(crypto_chip_serial_hex),
                 "%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                 chip_serial[0], chip_serial[1], chip_serial[2],
                 chip_serial[3], chip_serial[4], chip_serial[5],
                 chip_serial[6], chip_serial[7], chip_serial[8]);
        ESP_LOGI(TAG, "Chip serial: %s", crypto_chip_serial_hex);
    }

    // ── Read manufacturing data from OTP zone ─────────────────────────────
    macdap::ManufacturingData mfg = {};
    esp_err_t err = chip.read_manufacturing_data(&mfg);

    if (err == ESP_ERR_NOT_FOUND)
    {
        ESP_LOGW(TAG, "OTP zone not provisioned (magic mismatch) — manufacturing data unavailable");
        return;
    }

    if (err == ESP_ERR_INVALID_CRC)
    {
        ESP_LOGE(TAG, "OTP zone CRC32 mismatch — data may be corrupt");
        return;
    }

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read manufacturing data: 0x%x", err);
        return;
    }

    // ── Populate cache ────────────────────────────────────────────────────
    strncpy(crypto_device_serial,       mfg.serial,       sizeof(crypto_device_serial) - 1);
    strncpy(crypto_device_manufacturer, mfg.manufacturer, sizeof(crypto_device_manufacturer) - 1);
    strncpy(crypto_device_model,        mfg.model,        sizeof(crypto_device_model) - 1);
    snprintf(crypto_hw_revision, sizeof(crypto_hw_revision), "%d.%d",
             mfg.hw_rev_major, mfg.hw_rev_minor);

    s_provisioned = true;

    ESP_LOGI(TAG, "Manufacturing data loaded:");
    ESP_LOGI(TAG, "  Serial:       %s", crypto_device_serial);
    ESP_LOGI(TAG, "  Manufacturer: %s", crypto_device_manufacturer);
    ESP_LOGI(TAG, "  Model:        %s", crypto_device_model);
    ESP_LOGI(TAG, "  HW revision:  %s", crypto_hw_revision);
    ESP_LOGI(TAG, "  Mfg date:     %08lu", (unsigned long)mfg.mfg_date);

    // ── Register hardware TLS key (requires locked + provisioned chip) ────
    if (s_data_locked)
    {
        err = chip.init_tls_alt_key();
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "TLS hardware key active — ECDSA offloaded to ATECC608B");
        }
        else
        {
            ESP_LOGW(TAG, "TLS alt-key registration failed (0x%x) — using software fallback", err);
        }
    }
    else
    {
        ESP_LOGW(TAG, "Data zone not locked — TLS alt-key skipped (development mode)");
    }

    ESP_LOGI(TAG, "Crypto manager ready");
}
