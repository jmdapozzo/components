#pragma once

#include "esp_err.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "eventLoop.hpp"

#define GPS_MAX_SATELLITES_IN_USE (12)
#define GPS_MAX_SATELLITES_IN_VIEW (16)

ESP_EVENT_DECLARE_BASE(GPS_EVENTS);

#ifdef __cplusplus
extern "C"
{
#endif

namespace macdap
{
    typedef enum {
        GPS_UPDATE, /*!< GPS information has been updated */
        GPS_UNKNOWN /*!< Unknown statements detected */
    } nmea_event_id_t;

    typedef enum {
        GPS_FIX_INVALID, /*!< Not fixed */
        GPS_FIX_GPS,     /*!< GPS */
        GPS_FIX_DGPS,    /*!< Differential GPS */
    } gps_fix_t;

    typedef enum {
        GPS_MODE_INVALID = 1, /*!< Not fixed */
        GPS_MODE_2D,          /*!< 2D GPS */
        GPS_MODE_3D           /*!< 3D GPS */
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
        STATEMENT_UNKNOWN = 0, /*!< Unknown statement */
        STATEMENT_GGA,         /*!< GGA */
        STATEMENT_GSA,         /*!< GSA */
        STATEMENT_RMC,         /*!< RMC */
        STATEMENT_GSV,         /*!< GSV */
        STATEMENT_GLL,         /*!< GLL */
        STATEMENT_VTG,         /*!< VTG */
        STATEMENT_ZDA,         /*!< ZDA */
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
        GPS();
        ~GPS();
        void ReleaseResources();

    public:
        GPS(GPS const&) = delete;
        void operator=(GPS const &) = delete;
        static GPS &getInstance()
        {
            static GPS instance;
            return instance;
        }
        void getPosition();
    };
}

#ifdef __cplusplus
}
#endif