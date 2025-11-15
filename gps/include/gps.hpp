#pragma once

#include "esp_err.h"
#include "esp_event.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#define GPS_MAX_SATELLITES_IN_USE (12)
#define GPS_MAX_SATELLITES_IN_VIEW (16)

ESP_EVENT_DECLARE_BASE(GPS_EVENTS);

namespace macdap
{
    typedef enum {
        GpsUpdate, /*!< GPS information has been updated */
        GpsUnknown /*!< Unknown statements detected */
    } nmea_event_id_t;

    typedef enum {
        GpsFixInvalid, /*!< Not fixed */
        GpsFixGps,     /*!< GPS */
        GpsFixDgps,    /*!< Differential GPS */
    } gps_fix_t;

    typedef enum {
        GpsModeInvalid = 1, /*!< Not fixed */
        GpsMode2D,          /*!< 2D GPS */
        GpsMode3D           /*!< 3D GPS */
    } gps_fix_mode_t;

    typedef struct {
        uint8_t num;       /*!< Satellite number */
        uint8_t elevation; /*!< Satellite elevation */
        uint16_t azimuth;  /*!< Satellite azimuth */
        uint8_t snr;       /*!< Satellite signal noise ratio */
    } gps_satellite_t;

    typedef struct {
        uint8_t hour;      /*!< Hour */
        uint8_t minute;    /*!< Minute */
        uint8_t second;    /*!< Second */
        uint16_t thousand; /*!< Thousand */
    } gps_time_t;

    typedef struct {
        uint8_t day;   /*!< Day (start from 1) */
        uint8_t month; /*!< Month (start from 1) */
        uint16_t year; /*!< Year (start from 2000) */
    } gps_date_t;

    typedef enum {
        StatementUnknown = 0, /*!< Unknown statement */
        StatementGga,         /*!< GGA */
        StatementGsa,         /*!< GSA */
        StatementRmc,         /*!< RMC */
        StatementGsv,         /*!< GSV */
        StatementGll,         /*!< GLL */
        StatementVtg,         /*!< VTG */
        StatementZda,         /*!< ZDA */
        StatementTxt,         /*!< TXT */
    } nmea_statement_t;

    typedef struct {
        float latitude;                                                /*!< Latitude (degrees) */
        float longitude;                                               /*!< Longitude (degrees) */
        float altitude;                                                /*!< Altitude (meters) */
        gps_fix_t fix;                                                 /*!< Fix status */
        uint8_t sats_in_use;                                           /*!< Number of satellites in use */
        gps_time_t tim;                                                /*!< time in UTC */
        gps_fix_mode_t fix_mode;                                       /*!< Fix mode */
        uint8_t sats_id_in_use[GPS_MAX_SATELLITES_IN_USE];             /*!< ID list of satellite in use */
        float dop_h;                                                   /*!< Horizontal dilution of precision */
        float dop_p;                                                   /*!< Position dilution of precision  */
        float dop_v;                                                   /*!< Vertical dilution of precision  */
        uint8_t sats_in_view;                                          /*!< Number of satellites in view */
        gps_satellite_t sats_desc_in_view[GPS_MAX_SATELLITES_IN_VIEW]; /*!< Information of satellites in view */
        gps_date_t date;                                               /*!< Fix date */
        bool valid;                                                    /*!< GPS validity */
        float speed;                                                   /*!< Ground speed, unit: m/s */
        float cog;                                                     /*!< Course over ground */
        float variation;                                               /*!< Magnetic variation */
    } gps_t;

    class GPS
    {

    private:
        TaskHandle_t m_taskHandle = nullptr;
        GPS(esp_event_loop_handle_t event_loop_hdl);
        ~GPS();
        void ReleaseResources();

    public:
        GPS(GPS const&) = delete;
        void operator=(GPS const &) = delete;
        static GPS &get_instance(esp_event_loop_handle_t event_loop_hdl)
        {
            static GPS instance(event_loop_hdl);
            return instance;
        }
        gps_t get_gps_data();
        esp_err_t register_event_handler(esp_event_handler_t event_handler, void* handler_arg = nullptr);
        esp_err_t unregister_event_handler(esp_event_handler_t event_handler);
    };
}
