#pragma once

#include "esp_err.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <mbedtls/pk.h>
#include <cstdint>
#include <cstddef>

namespace macdap
{
    // ATECC608B slot numbers
    static constexpr uint8_t ATECC608B_SLOT_PRIVATE_KEY   = 0;   // ECC P-256 private key (never readable)
    static constexpr uint8_t ATECC608B_SLOT_DEVICE_CERT   = 10;  // Device certificate (DER)
    static constexpr uint8_t ATECC608B_SLOT_SIGNER_CERT   = 11;  // Signer / intermediate cert (DER)
    static constexpr uint8_t ATECC608B_SLOT_ROOT_CERT     = 12;  // Root CA cert (optional)

    // OTP zone size
    static constexpr size_t ATECC608B_OTP_SIZE = 64;

    // Device serial number size (chip-internal, always readable)
    static constexpr size_t ATECC608B_CHIP_SERIAL_SIZE = 9;

    // ECC public key size (uncompressed, without 0x04 prefix: X || Y)
    static constexpr size_t ATECC608B_PUBLIC_KEY_SIZE = 64;

    // ECDSA signature size (R || S)
    static constexpr size_t ATECC608B_SIGNATURE_SIZE = 64;

    // SHA-256 digest size
    static constexpr size_t ATECC608B_DIGEST_SIZE = 32;

    /**
     * @brief Manufacturing data stored in the OTP zone (64 bytes total).
     *
     * This struct is written once during factory provisioning and becomes
     * permanently read-only after the OTP zone is locked.
     *
     * Layout:
     *   Offset  Len  Field
     *   ------  ---  -----
     *   0x00     4   magic (CONFIG_ATECC608B_OTP_MAGIC)
     *   0x04    16   serial number (null-terminated ASCII)
     *   0x14    16   manufacturer name (null-terminated ASCII)
     *   0x24    16   hardware model (null-terminated ASCII)
     *   0x34     1   hw_rev_major
     *   0x35     1   hw_rev_minor
     *   0x36     2   reserved (zero)
     *   0x38     4   manufacturing date (YYYYMMDD as uint32_t, big-endian)
     *   0x3C     4   CRC32 of bytes 0x00..0x3B
     *   Total:  64 bytes
     */
    struct __attribute__((packed)) ManufacturingData
    {
        uint32_t magic;            // Validity marker
        char     serial[16];       // Device serial number
        char     manufacturer[16]; // Manufacturer name
        char     model[16];        // Hardware model
        uint8_t  hw_rev_major;     // Hardware revision major
        uint8_t  hw_rev_minor;     // Hardware revision minor
        uint8_t  reserved[2];      // Future use, must be 0x00
        uint32_t mfg_date;         // Manufacturing date YYYYMMDD (big-endian)
        uint32_t crc32;            // CRC32 of bytes [0..59]
    };
    static_assert(sizeof(ManufacturingData) == ATECC608B_OTP_SIZE,
                  "ManufacturingData must be exactly 64 bytes");

    class SecureElement
    {
    private:
        bool m_is_present;
        bool m_config_locked;
        bool m_data_locked;
        SemaphoreHandle_t m_semaphore_handle;

        SecureElement();
        ~SecureElement();

        esp_err_t check_lock_status();

    public:
        SecureElement(SecureElement const&) = delete;
        void operator=(SecureElement const&) = delete;

        static SecureElement& get_instance()
        {
            static SecureElement instance;
            return instance;
        }

        bool is_present() const { return m_is_present; }
        bool is_config_locked() const { return m_config_locked; }
        bool is_data_locked() const { return m_data_locked; }

        // ── Chip identity ──────────────────────────────────────────────────
        /**
         * @brief Read the chip's factory-burned 9-byte serial number.
         *        Always readable regardless of lock state.
         */
        esp_err_t get_chip_serial(uint8_t serial[ATECC608B_CHIP_SERIAL_SIZE]);

        // ── OTP zone — manufacturing data ──────────────────────────────────
        /**
         * @brief Read 64 bytes from the OTP zone into @p data.
         */
        esp_err_t read_otp(uint8_t data[ATECC608B_OTP_SIZE]);

        /**
         * @brief Write 64 bytes to the OTP zone.
         *        Only valid before the OTP zone is locked (before lock_data_zone()).
         */
        esp_err_t write_otp(const uint8_t data[ATECC608B_OTP_SIZE]);

        /**
         * @brief Read and deserialise the ManufacturingData struct from OTP.
         *        Validates the magic number and CRC32.
         *
         * @return ESP_OK              – struct populated successfully
         *         ESP_ERR_NOT_FOUND   – magic number mismatch (not provisioned)
         *         ESP_ERR_INVALID_CRC – CRC32 mismatch (data corrupt)
         */
        esp_err_t read_manufacturing_data(ManufacturingData* data);

        /**
         * @brief Serialise and write ManufacturingData to OTP zone.
         *        Fills in magic and CRC32 automatically.
         *        Only valid before lock_data_zone().
         */
        esp_err_t write_manufacturing_data(const ManufacturingData* data);

        // ── Data zone — certificates ───────────────────────────────────────
        /**
         * @brief Read DER-encoded certificate from @p slot.
         *        @p len is in/out: provide buffer size, receives bytes written.
         */
        esp_err_t read_cert(uint8_t slot, uint8_t* der, size_t* len);

        /**
         * @brief Write DER-encoded certificate to @p slot.
         *        Only valid before lock_data_zone().
         */
        esp_err_t write_cert(uint8_t slot, const uint8_t* der, size_t len);

        // ── Key operations ─────────────────────────────────────────────────
        /**
         * @brief Generate a new ECC P-256 key pair in @p slot.
         *        The private key remains in the chip and is never exported.
         *        @p public_key (64 bytes, X||Y) is returned to the caller.
         */
        esp_err_t gen_key(uint8_t slot, uint8_t public_key[ATECC608B_PUBLIC_KEY_SIZE]);

        /**
         * @brief Sign a 32-byte @p digest with the private key in @p slot.
         *        The resulting ECDSA signature (R||S, 64 bytes) is in @p signature.
         */
        esp_err_t sign(uint8_t slot,
                       const uint8_t digest[ATECC608B_DIGEST_SIZE],
                       uint8_t signature[ATECC608B_SIGNATURE_SIZE]);

        // ── Zone locking (manufacturing use only) ─────────────────────────
        /**
         * @brief Lock the config zone. IRREVERSIBLE.
         *        Must be called before lock_data_zone().
         */
        esp_err_t lock_config_zone();

        /**
         * @brief Lock the data zone and OTP zone. IRREVERSIBLE.
         *        Config zone must be locked first.
         */
        esp_err_t lock_data_zone();

        // ── TLS integration ────────────────────────────────────────────────
        /**
         * @brief Verify that slot 0 holds a valid ECC key (TLS readiness check).
         *        Reads the public key from the slot without exposing the private key.
         *        Returns ESP_OK if the key is present, ESP_FAIL otherwise.
         *        Call once at startup to confirm the chip is provisioned for TLS.
         */
        esp_err_t init_tls_alt_key();

        /**
         * @brief Initialise an mbedTLS pk_context backed by the hardware private key.
         *
         *        After this call, @p ctx represents the device private key stored in
         *        slot 0. ECDSA operations performed via @p ctx are executed inside the
         *        ATECC608B — the private key never leaves the chip.
         *
         *        Call this function each time a TLS connection is established:
         *        @code
         *            mbedtls_pk_context key;
         *            chip.init_tls_pk_context(&key);
         *            // pass &key to esp_tls or mbedtls_ssl_conf_own_cert()
         *        @endcode
         *
         *        The caller is responsible for calling mbedtls_pk_free(&ctx) when done.
         */
        esp_err_t init_tls_pk_context(mbedtls_pk_context* ctx);
    };
}
