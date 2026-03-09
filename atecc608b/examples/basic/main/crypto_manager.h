#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ── Manufacturing data cache (populated at boot from ATECC608B OTP zone) ──────

// Hardware serial number (max 15 chars + null terminator)
extern char crypto_device_serial[16];

// Manufacturer name (max 15 chars + null terminator)
extern char crypto_device_manufacturer[16];

// Hardware model string (max 15 chars + null terminator)
extern char crypto_device_model[16];

// Hardware revision in "major.minor" format (e.g. "1.0")
extern char crypto_hw_revision[8];

// ATECC608B chip serial number as a hex string (9 bytes = 18 hex chars + null)
extern char crypto_chip_serial_hex[19];

// ── Lifecycle ─────────────────────────────────────────────────────────────────

/**
 * @brief Initialize the crypto manager.
 *
 * Must be called after init_i2c() and before any TLS operations.
 *
 * - Detects the ATECC608B on the I2C bus.
 * - Reads chip serial number (always available).
 * - If the OTP zone is provisioned (valid magic + CRC32):
 *     - Populates the crypto_* cache variables above.
 *     - Calls init_tls_alt_key() to register the hardware ECDSA engine.
 * - If not provisioned, the cache variables remain empty strings and
 *   TLS falls back to software keys.
 */
void init_crypto_manager(void);

/**
 * @brief Returns true if the chip was found on the I2C bus.
 */
bool crypto_manager_chip_present(void);

/**
 * @brief Returns true if the chip has been provisioned with manufacturing data
 *        (OTP zone written and locked, valid magic + CRC32).
 */
bool crypto_manager_is_provisioned(void);

/**
 * @brief Returns true if the ATECC608B data zone is locked.
 *        When locked, write operations to OTP/data slots are permanent.
 */
bool crypto_manager_data_locked(void);

#ifdef __cplusplus
}
#endif
