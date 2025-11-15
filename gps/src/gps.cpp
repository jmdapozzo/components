#include <gps.hpp>
#include <freertos/FreeRTOS.h>
#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <driver/uart.h>

#define NMEA_PARSER_RUNTIME_BUFFER_SIZE (CONFIG_GPS_RING_BUFFER_SIZE / 2)
#define NMEA_MAX_STATEMENT_ITEM_LENGTH (16)
#define NMEA_EVENT_LOOP_QUEUE_SIZE (16)
#define UART_EVENT_QUEUE_SIZE (16)

using namespace macdap;

typedef struct {
    uint8_t item_pos;                              /*!< Current position in item */
    uint8_t item_num;                              /*!< Current item number */
    uint8_t asterisk;                              /*!< Asterisk detected flag */
    uint8_t crc;                                   /*!< Calculated CRC value */
    uint8_t parsed_statement;                      /*!< OR'd of statements that have been parsed */
    uint8_t sat_num;                               /*!< Satellite number */
    uint8_t sat_count;                             /*!< Satellite count */
    uint8_t cur_statement;                         /*!< Current statement ID */
    uint32_t all_statements;                       /*!< All statements mask */
    char item_str[NMEA_MAX_STATEMENT_ITEM_LENGTH]; /*!< Current item */
    gps_t parent;                                  /*!< Parent class */
    uint8_t *buffer;                               /*!< Runtime buffer */
    esp_event_loop_handle_t event_loop_hdl;        /*!< Event loop handle */
    SemaphoreHandle_t semaphore_handle;             /*!< Semaphore handle */
    QueueHandle_t event_queue;                     /*!< UART event queue handle */
} esp_gps_t;

static const char *TAG = "gps";
ESP_EVENT_DEFINE_BASE(GPS_EVENTS);
static esp_gps_t *_esp_gps = NULL;

static float parse_lat_long(esp_gps_t *esp_gps)
{
    float ll = strtof(esp_gps->item_str, NULL);
    int deg = ((int)ll) / 100;
    float min = ll - (deg * 100);
    ll = deg + min / 60.0f;
    return ll;
}

static inline uint8_t convert_two_digit2number(const char *digit_char)
{
    return 10 * (digit_char[0] - '0') + (digit_char[1] - '0');
}

static inline uint16_t convert_four_digit2number(const char *digit_char)
{
    return 1000 * (digit_char[0] - '0') + 100 * (digit_char[1] - '0') + 10 * (digit_char[2] - '0') + (digit_char[3] - '0');
}

static void parse_utc_time(esp_gps_t *esp_gps)
{
    esp_gps->parent.tim.hour = convert_two_digit2number(esp_gps->item_str + 0);
    esp_gps->parent.tim.minute = convert_two_digit2number(esp_gps->item_str + 2);
    esp_gps->parent.tim.second = convert_two_digit2number(esp_gps->item_str + 4);
    if (esp_gps->item_str[6] == '.') {
        uint16_t tmp = 0;
        uint8_t i = 7;
        while (esp_gps->item_str[i]) {
            tmp = 10 * tmp + esp_gps->item_str[i] - '0';
            i++;
        }
        esp_gps->parent.tim.thousand = tmp;
    }
}

#if CONFIG_NMEA_STATEMENT_GGA
static void parse_gga(esp_gps_t *esp_gps)
{
    /* Process GGA statement */
    switch (esp_gps->item_num) {
    case 1: /* Process UTC time */
        parse_utc_time(esp_gps);
        break;
    case 2: /* Latitude */
        esp_gps->parent.latitude = parse_lat_long(esp_gps);
        break;
    case 3: /* Latitude north(1)/south(-1) information */
        if (esp_gps->item_str[0] == 'S' || esp_gps->item_str[0] == 's') {
            esp_gps->parent.latitude *= -1;
        }
        break;
    case 4: /* Longitude */
        esp_gps->parent.longitude = parse_lat_long(esp_gps);
        break;
    case 5: /* Longitude east(1)/west(-1) information */
        if (esp_gps->item_str[0] == 'W' || esp_gps->item_str[0] == 'w') {
            esp_gps->parent.longitude *= -1;
        }
        break;
    case 6: /* Fix status */
        esp_gps->parent.fix = (gps_fix_t)strtol(esp_gps->item_str, NULL, 10);
        break;
    case 7: /* Satellites in use */
        esp_gps->parent.sats_in_use = (uint8_t)strtol(esp_gps->item_str, NULL, 10);
        break;
    case 8: /* HDOP */
        esp_gps->parent.dop_h = strtof(esp_gps->item_str, NULL);
        break;
    case 9: /* Altitude */
        esp_gps->parent.altitude = strtof(esp_gps->item_str, NULL);
        break;
    case 11: /* Altitude above ellipsoid */
        esp_gps->parent.altitude += strtof(esp_gps->item_str, NULL);
        break;
    default:
        break;
    }
}
#endif

#if CONFIG_NMEA_STATEMENT_GSA
static void parse_gsa(esp_gps_t *esp_gps)
{
    /* Process GSA statement */
    switch (esp_gps->item_num) {
    case 2: /* Process fix mode */
        esp_gps->parent.fix_mode = (gps_fix_mode_t)strtol(esp_gps->item_str, NULL, 10);
        break;
    case 15: /* Process PDOP */
        esp_gps->parent.dop_p = strtof(esp_gps->item_str, NULL);
        break;
    case 16: /* Process HDOP */
        esp_gps->parent.dop_h = strtof(esp_gps->item_str, NULL);
        break;
    case 17: /* Process VDOP */
        esp_gps->parent.dop_v = strtof(esp_gps->item_str, NULL);
        break;
    default:
        /* Parse satellite IDs */
        if (esp_gps->item_num >= 3 && esp_gps->item_num <= 14) {
            esp_gps->parent.sats_id_in_use[esp_gps->item_num - 3] = (uint8_t)strtol(esp_gps->item_str, NULL, 10);
        }
        break;
    }
}
#endif

#if CONFIG_NMEA_STATEMENT_GSV
static void parse_gsv(esp_gps_t *esp_gps)
{
    /* Process GSV statement */
    switch (esp_gps->item_num) {
    case 1: /* total GSV numbers */
        esp_gps->sat_count = (uint8_t)strtol(esp_gps->item_str, NULL, 10);
        break;
    case 2: /* Current GSV statement number */
        esp_gps->sat_num = (uint8_t)strtol(esp_gps->item_str, NULL, 10);
        break;
    case 3: /* Process satellites in view */
        esp_gps->parent.sats_in_view = (uint8_t)strtol(esp_gps->item_str, NULL, 10);
        break;
    default:
        if (esp_gps->item_num >= 4 && esp_gps->item_num <= 19) {
            uint8_t item_num = esp_gps->item_num - 4; /* Normalize item number from 4-19 to 0-15 */
            uint8_t index;
            uint32_t value;
            index = 4 * (esp_gps->sat_num - 1) + item_num / 4; /* Get array index */
            if (index < GPS_MAX_SATELLITES_IN_VIEW) {
                value = strtol(esp_gps->item_str, NULL, 10);
                switch (item_num % 4) {
                case 0:
                    esp_gps->parent.sats_desc_in_view[index].num = (uint8_t)value;
                    break;
                case 1:
                    esp_gps->parent.sats_desc_in_view[index].elevation = (uint8_t)value;
                    break;
                case 2:
                    esp_gps->parent.sats_desc_in_view[index].azimuth = (uint16_t)value;
                    break;
                case 3:
                    esp_gps->parent.sats_desc_in_view[index].snr = (uint8_t)value;
                    break;
                default:
                    break;
                }
            }
        }
        break;
    }
}
#endif

#if CONFIG_NMEA_STATEMENT_RMC
static void parse_rmc(esp_gps_t *esp_gps)
{
    /* Process GPRMC statement */
    switch (esp_gps->item_num) {
    case 1:/* Process UTC time */
        parse_utc_time(esp_gps);
        break;
    case 2: /* Process valid status */
        esp_gps->parent.valid = (esp_gps->item_str[0] == 'A');
        break;
    case 3:/* Latitude */
        esp_gps->parent.latitude = parse_lat_long(esp_gps);
        break;
    case 4: /* Latitude north(1)/south(-1) information */
        if (esp_gps->item_str[0] == 'S' || esp_gps->item_str[0] == 's') {
            esp_gps->parent.latitude *= -1;
        }
        break;
    case 5: /* Longitude */
        esp_gps->parent.longitude = parse_lat_long(esp_gps);
        break;
    case 6: /* Longitude east(1)/west(-1) information */
        if (esp_gps->item_str[0] == 'W' || esp_gps->item_str[0] == 'w') {
            esp_gps->parent.longitude *= -1;
        }
        break;
    case 7: /* Process ground speed in unit m/s */
        esp_gps->parent.speed = strtof(esp_gps->item_str, NULL) * 1.852;
        break;
    case 8: /* Process true course over ground */
        esp_gps->parent.cog = strtof(esp_gps->item_str, NULL);
        break;
    case 9: /* Process date */
        esp_gps->parent.date.day = convert_two_digit2number(esp_gps->item_str + 0);
        esp_gps->parent.date.month = convert_two_digit2number(esp_gps->item_str + 2);
        esp_gps->parent.date.year = convert_two_digit2number(esp_gps->item_str + 4);
        break;
    case 10: /* Process magnetic variation */
        esp_gps->parent.variation = strtof(esp_gps->item_str, NULL);
        break;
    default:
        break;
    }
}
#endif

#if CONFIG_NMEA_STATEMENT_GLL
static void parse_gll(esp_gps_t *esp_gps)
{
    /* Process GPGLL statement */
    switch (esp_gps->item_num) {
    case 1:/* Latitude */
        esp_gps->parent.latitude = parse_lat_long(esp_gps);
        break;
    case 2: /* Latitude north(1)/south(-1) information */
        if (esp_gps->item_str[0] == 'S' || esp_gps->item_str[0] == 's') {
            esp_gps->parent.latitude *= -1;
        }
        break;
    case 3: /* Longitude */
        esp_gps->parent.longitude = parse_lat_long(esp_gps);
        break;
    case 4: /* Longitude east(1)/west(-1) information */
        if (esp_gps->item_str[0] == 'W' || esp_gps->item_str[0] == 'w') {
            esp_gps->parent.longitude *= -1;
        }
        break;
    case 5:/* Process UTC time */
        parse_utc_time(esp_gps);
        break;
    case 6: /* Process valid status */
        esp_gps->parent.valid = (esp_gps->item_str[0] == 'A');
        break;
    default:
        break;
    }
}
#endif

#if CONFIG_NMEA_STATEMENT_VTG
static void parse_vtg(esp_gps_t *esp_gps)
{
    /* Process GPVGT statement */
    switch (esp_gps->item_num) {
    case 1: /* Process true course over ground */
        esp_gps->parent.cog = strtof(esp_gps->item_str, NULL);
        break;
    case 3:/* Process magnetic variation */
        esp_gps->parent.variation = strtof(esp_gps->item_str, NULL);
        break;
    case 5:/* Process ground speed in unit m/s */
        esp_gps->parent.speed = strtof(esp_gps->item_str, NULL) * 1.852;//knots to m/s
        break;
    case 7:/* Process ground speed in unit m/s */
        esp_gps->parent.speed = strtof(esp_gps->item_str, NULL) / 3.6;//km/h to m/s
        break;
    default:
        break;
    }
}
#endif

#if CONFIG_NMEA_STATEMENT_ZDA
static void parse_zda(esp_gps_t *esp_gps)
{
    /* Process GNZDA statement */
    switch (esp_gps->item_num) {
    case 1: /* Process UTC time */
        parse_utc_time(esp_gps);
        break;
    case 2:/* Process day */
        esp_gps->parent.date.day = convert_two_digit2number(esp_gps->item_str);
        break;
    case 3:/* Process month */
        esp_gps->parent.date.month = convert_two_digit2number(esp_gps->item_str);
        break;
    case 4:/* Process year */
        esp_gps->parent.date.year = convert_four_digit2number(esp_gps->item_str);
        break;
    default:
        break;
    }
}
#endif

#if CONFIG_NMEA_STATEMENT_TXT
static void parse_txt(esp_gps_t *esp_gps)
{
    /* Process GNTXT statement */
    static uint8_t txt_type = 0;
    static char txt_message[128] = {0};
    
    switch (esp_gps->item_num) {
    case 1: /* Total number of sentences for this message */
        break;
    case 2: /* Sentence number */
        break;
    case 3: /* Text identifier (00=Error, 01=Warning, 02=Notice, 07=User) */
        txt_type = (uint8_t)strtol(esp_gps->item_str, NULL, 10);
        break;
    case 4: /* Text message */
        strncpy(txt_message, esp_gps->item_str, sizeof(txt_message) - 1);
        txt_message[sizeof(txt_message) - 1] = '\0';
        
        /* Log message according to its type */
        switch (txt_type) {
        case 0: /* Error */
            ESP_LOGW(TAG, "GPS TXT Generic: %s", txt_message);
            break;
        case 1: /* Error */
            ESP_LOGE(TAG, "GPS TXT Error: %s", txt_message);
            break;
        case 2: /* Information */
            ESP_LOGI(TAG, "GPS TXT Information: %s", txt_message);
            break;
        default: /* Unknown type */
            ESP_LOGE(TAG, "GPS TXT Unknown type %d: %s", txt_type, txt_message);
            break;
        }
        break;
    default:
        break;
    }
}
#endif

static esp_err_t parse_item(esp_gps_t *esp_gps)
{
    esp_err_t err = ESP_OK;
    /* start of a statement */
    if (esp_gps->item_num == 0 && esp_gps->item_str[0] == '$') {
        if (0) {
        }
#if CONFIG_NMEA_STATEMENT_GGA
        else if (strstr(esp_gps->item_str, "GGA")) {
            esp_gps->cur_statement = StatementGga;
        }
#endif
#if CONFIG_NMEA_STATEMENT_GSA
        else if (strstr(esp_gps->item_str, "GSA")) {
            esp_gps->cur_statement = StatementGsa;
        }
#endif
#if CONFIG_NMEA_STATEMENT_RMC
        else if (strstr(esp_gps->item_str, "RMC")) {
            esp_gps->cur_statement = StatementRmc;
        }
#endif
#if CONFIG_NMEA_STATEMENT_GSV
        else if (strstr(esp_gps->item_str, "GSV")) {
            esp_gps->cur_statement = StatementGsv;
        }
#endif
#if CONFIG_NMEA_STATEMENT_GLL
        else if (strstr(esp_gps->item_str, "GLL")) {
            esp_gps->cur_statement = StatementGll;
        }
#endif
#if CONFIG_NMEA_STATEMENT_VTG
        else if (strstr(esp_gps->item_str, "VTG")) {
            esp_gps->cur_statement = StatementVtg;
        }
#endif
#if CONFIG_NMEA_STATEMENT_ZDA
        else if (strstr(esp_gps->item_str, "ZDA")) {
            esp_gps->cur_statement = StatementZda;
        }
#endif
#if CONFIG_NMEA_STATEMENT_TXT
        else if (strstr(esp_gps->item_str, "TXT")) {
            esp_gps->cur_statement = StatementTxt;
        }
#endif
        else {
            esp_gps->cur_statement = StatementUnknown;
        }
        goto out;
    }
    /* Parse each item, depend on the type of the statement */
    if (esp_gps->cur_statement == StatementUnknown) {
        goto out;
    }
#if CONFIG_NMEA_STATEMENT_GGA
    else if (esp_gps->cur_statement == StatementGga) {
        parse_gga(esp_gps);
    }
#endif
#if CONFIG_NMEA_STATEMENT_GSA
    else if (esp_gps->cur_statement == StatementGsa) {
        parse_gsa(esp_gps);
    }
#endif
#if CONFIG_NMEA_STATEMENT_GSV
    else if (esp_gps->cur_statement == StatementGsv) {
        parse_gsv(esp_gps);
    }
#endif
#if CONFIG_NMEA_STATEMENT_RMC
    else if (esp_gps->cur_statement == StatementRmc) {
        parse_rmc(esp_gps);
    }
#endif
#if CONFIG_NMEA_STATEMENT_GLL
    else if (esp_gps->cur_statement == StatementGll) {
        parse_gll(esp_gps);
    }
#endif
#if CONFIG_NMEA_STATEMENT_VTG
    else if (esp_gps->cur_statement == StatementVtg) {
        parse_vtg(esp_gps);
    }
#endif
#if CONFIG_NMEA_STATEMENT_ZDA
    else if (esp_gps->cur_statement == StatementZda) {
        parse_zda(esp_gps);
    }
#endif
#if CONFIG_NMEA_STATEMENT_TXT
    else if (esp_gps->cur_statement == StatementTxt) {
        parse_txt(esp_gps);
    }
#endif
    else {
        err =  ESP_FAIL;
    }
out:
    return err;
}

static esp_err_t gps_decode(esp_gps_t *esp_gps, size_t len)
{
    const uint8_t *d = esp_gps->buffer;
    while (*d) {
        /* Start of a statement */
        if (*d == '$') {
            /* Reset runtime information */
            esp_gps->asterisk = 0;
            esp_gps->item_num = 0;
            esp_gps->item_pos = 0;
            esp_gps->cur_statement = 0;
            esp_gps->crc = 0;
            esp_gps->sat_count = 0;
            esp_gps->sat_num = 0;
            /* Add character to item */
            esp_gps->item_str[esp_gps->item_pos++] = *d;
            esp_gps->item_str[esp_gps->item_pos] = '\0';
        }
        /* Detect item separator character */
        else if (*d == ',') {
            /* Parse current item */
            parse_item(esp_gps);
            /* Add character to CRC computation */
            esp_gps->crc ^= (uint8_t)(*d);
            /* Start with next item */
            esp_gps->item_pos = 0;
            esp_gps->item_str[0] = '\0';
            esp_gps->item_num++;
        }
        /* End of CRC computation */
        else if (*d == '*') {
            /* Parse current item */
            parse_item(esp_gps);
            /* Asterisk detected */
            esp_gps->asterisk = 1;
            /* Start with next item */
            esp_gps->item_pos = 0;
            esp_gps->item_str[0] = '\0';
            esp_gps->item_num++;
        }
        /* End of statement */
        else if (*d == '\r') {
            /* Convert received CRC from string (hex) to number */
            uint8_t crc = (uint8_t)strtol(esp_gps->item_str, NULL, 16);
            /* CRC passed */
            if (esp_gps->crc == crc) {
                switch (esp_gps->cur_statement) {
#if CONFIG_NMEA_STATEMENT_GGA
                case StatementGga:
                    esp_gps->parsed_statement |= 1 << StatementGga;
                    break;
#endif
#if CONFIG_NMEA_STATEMENT_GSA
                case StatementGsa:
                    esp_gps->parsed_statement |= 1 << StatementGsa;
                    break;
#endif
#if CONFIG_NMEA_STATEMENT_RMC
                case StatementRmc:
                    esp_gps->parsed_statement |= 1 << StatementRmc;
                    break;
#endif
#if CONFIG_NMEA_STATEMENT_GSV
                case StatementGsv:
                    if (esp_gps->sat_num == esp_gps->sat_count) {
                        esp_gps->parsed_statement |= 1 << StatementGsv;
                    }
                    break;
#endif
#if CONFIG_NMEA_STATEMENT_GLL
                case StatementGll:
                    esp_gps->parsed_statement |= 1 << StatementGll;
                    break;
#endif
#if CONFIG_NMEA_STATEMENT_VTG
                case StatementVtg:
                    esp_gps->parsed_statement |= 1 << StatementVtg;
                    break;
#endif
#if CONFIG_NMEA_STATEMENT_ZDA
                case StatementZda:
                    esp_gps->parsed_statement |= 1 << StatementZda;
                    break;
#endif
#if CONFIG_NMEA_STATEMENT_TXT
                case StatementTxt:
                    // TXT statements are processed immediately and don't affect GPS update cycle
                    break;
#endif
                default:
                    break;
                }
                /* Check if all statements have been parsed */
                if (((esp_gps->parsed_statement) & esp_gps->all_statements) == esp_gps->all_statements) {
                    esp_gps->parsed_statement = 0;
                    /* Send signal to notify that GPS information has been updated */
                    esp_event_post_to(esp_gps->event_loop_hdl, GPS_EVENTS, GpsUpdate, &(esp_gps->parent), sizeof(gps_t), 100 / portTICK_PERIOD_MS);
                }
            } else {
                ESP_LOGD(TAG, "CRC Error for statement:%s", esp_gps->buffer);
            }
            if (esp_gps->cur_statement == StatementUnknown) {
                /* Send signal to notify that one unknown statement has been met */
                esp_event_post_to(esp_gps->event_loop_hdl, GPS_EVENTS, GpsUnknown, esp_gps->buffer, len, 100 / portTICK_PERIOD_MS);
            }
        }
        /* Other non-space character */
        else {
            if (!(esp_gps->asterisk)) {
                /* Add to CRC */
                esp_gps->crc ^= (uint8_t)(*d);
            }
            /* Add character to item */
            esp_gps->item_str[esp_gps->item_pos++] = *d;
            esp_gps->item_str[esp_gps->item_pos] = '\0';
        }
        /* Process next character */
        d++;
    }
    return ESP_OK;
}

static void esp_handle_uart_pattern(esp_gps_t *esp_gps)
{
    int pos = uart_pattern_pop_pos(static_cast<uart_port_t>(static_cast<uart_port_t>(CONFIG_GPS_UART_NUMBER)));
    if (pos != -1) {
        int read_len = uart_read_bytes(static_cast<uart_port_t>(CONFIG_GPS_UART_NUMBER), esp_gps->buffer, pos + 1, 100 / portTICK_PERIOD_MS);
        esp_gps->buffer[read_len] = '\0';
        if (gps_decode(esp_gps, read_len + 1) != ESP_OK) {
            ESP_LOGW(TAG, "GPS decode line failed");
        }
    } else {
        ESP_LOGW(TAG, "Pattern Queue Size too small");
        uart_flush_input(static_cast<uart_port_t>(CONFIG_GPS_UART_NUMBER));
    }
}

static void nmeaParserTask(void *arg)
{
    esp_gps_t *esp_gps = (esp_gps_t *)arg;
    uart_event_t event;
    while (1) {
        if (xQueueReceive(esp_gps->event_queue, &event, pdMS_TO_TICKS(200))) {
            switch (event.type) {
            case UART_DATA:
                break;
            case UART_FIFO_OVF:
                ESP_LOGW(TAG, "HW FIFO Overflow");
                uart_flush(static_cast<uart_port_t>(CONFIG_GPS_UART_NUMBER));
                xQueueReset(esp_gps->event_queue);
                break;
            case UART_BUFFER_FULL:
                ESP_LOGW(TAG, "Ring Buffer Full");
                uart_flush(static_cast<uart_port_t>(CONFIG_GPS_UART_NUMBER));
                xQueueReset(esp_gps->event_queue);
                break;
            case UART_BREAK:
                ESP_LOGW(TAG, "Rx Break");
                break;
            case UART_PARITY_ERR:
                ESP_LOGE(TAG, "Parity Error");
                break;
            case UART_FRAME_ERR:
                ESP_LOGE(TAG, "Frame Error");
                break;
            case UART_PATTERN_DET:
                esp_handle_uart_pattern(esp_gps);
                break;
            default:
                ESP_LOGW(TAG, "unknown uart event type: %d", event.type);
                break;
            }
        }
    }
}

void GPS::ReleaseResources()
{
    if (_esp_gps) {
        if (_esp_gps->buffer) {
            free(_esp_gps->buffer);
        }
        free(_esp_gps);
    }

    uart_driver_delete(static_cast<uart_port_t>(CONFIG_GPS_UART_NUMBER));

    if (_esp_gps->semaphore_handle) {
        vSemaphoreDelete(_esp_gps->semaphore_handle);
    }

    if (m_taskHandle) {
        vTaskDelete(m_taskHandle);
    }
}

#if CONFIG_GPS_EVENT_LOG
static void onGPSEvent(void* handler_arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    macdap::gps_t *gps = static_cast<macdap::gps_t*>(event_data);

    switch (event_id) {
    case macdap::GpsUpdate:
        ESP_LOGI(TAG, "%d/%d/%d %d:%d:%d => \r\n"
                 "\t\t\t\t\t\tlatitude   = %.05f°N\r\n"
                 "\t\t\t\t\t\tlongitude  = %.05f°E\r\n"
                 "\t\t\t\t\t\taltitude   = %.02fm\r\n"
                 "\t\t\t\t\t\tspeed      = %fm/s",
                 gps->date.year, gps->date.month, gps->date.day,
                 gps->tim.hour, gps->tim.minute, gps->tim.second,
                 gps->latitude, gps->longitude, gps->altitude, gps->speed);
        break;
    case macdap::GpsUnknown:
        /* print unknown statements */
        ESP_LOGW(TAG, "Unknown statement:%s", (char *)event_data);
        break;
    default:
        break;
    }
}
#endif

GPS::GPS(esp_event_loop_handle_t event_loop_hdl)
{
    ESP_LOGI(TAG, "Initializing...");

    if (!event_loop_hdl) {
        ESP_LOGE(TAG, "Event loop handle is required but not provided");
        return;
    }

    if (CONFIG_GPS_PPS != -1)
    {
        ESP_LOGW(TAG, "PPS is not yet supported, use -1 to disable");
    }

    esp_gps_t *esp_gps = (esp_gps_t *)calloc(1, sizeof(esp_gps_t));
    if (!esp_gps) {
        ESP_LOGE(TAG, "calloc memory for esp_fps failed");
        ReleaseResources();
        return;
    }
    esp_gps->buffer = (uint8_t *)calloc(1, NMEA_PARSER_RUNTIME_BUFFER_SIZE);
    if (!esp_gps->buffer) {
        ESP_LOGE(TAG, "calloc memory for runtime buffer failed");
        ReleaseResources();
        return;
    }

    esp_gps->semaphore_handle = xSemaphoreCreateMutex();
    if (!esp_gps->semaphore_handle)
    {
        ESP_LOGE(TAG, "xSemaphoreCreateMutex failed");
        ReleaseResources();
        return;
    }

    // Use the provided event loop handle
    esp_gps->event_loop_hdl = event_loop_hdl;

#if CONFIG_GPS_EVENT_LOG
    esp_event_handler_register_with(esp_gps->event_loop_hdl, GPS_EVENTS, ESP_EVENT_ANY_ID, onGPSEvent, NULL);
#endif

#if CONFIG_NMEA_STATEMENT_GSA
    esp_gps->all_statements |= (1 << StatementGsa);
#endif
#if CONFIG_NMEA_STATEMENT_GSV
    esp_gps->all_statements |= (1 << StatementGsv);
#endif
#if CONFIG_NMEA_STATEMENT_GGA
    esp_gps->all_statements |= (1 << StatementGga);
#endif
#if CONFIG_NMEA_STATEMENT_RMC
    esp_gps->all_statements |= (1 << StatementRmc);
#endif
#if CONFIG_NMEA_STATEMENT_GLL
    esp_gps->all_statements |= (1 << StatementGll);
#endif
#if CONFIG_NMEA_STATEMENT_VTG
    esp_gps->all_statements |= (1 << StatementVtg);
#endif
#if CONFIG_NMEA_STATEMENT_ZDA
    esp_gps->all_statements |= (1 << StatementZda);
#endif
#if CONFIG_NMEA_STATEMENT_ZDA
    esp_gps->all_statements |= (1 << StatementZda);
#endif

    // Exclude StatementUnknown from mandatory statements
    // TXT statements are not added to all_statements as they are informational only
    esp_gps->all_statements &= ~(1 << StatementUnknown);

    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    if (uart_driver_install(static_cast<uart_port_t>(CONFIG_GPS_UART_NUMBER), CONFIG_GPS_RING_BUFFER_SIZE, 0, UART_EVENT_QUEUE_SIZE, &esp_gps->event_queue, 0) != ESP_OK) {
        ESP_LOGE(TAG, "install uart driver failed");
        ReleaseResources();
        return;
    }
    if (uart_param_config(static_cast<uart_port_t>(CONFIG_GPS_UART_NUMBER), &uart_config) != ESP_OK) {
        ESP_LOGE(TAG, "config uart parameter failed");
        ReleaseResources();
        return;
    }
    if (uart_set_pin(static_cast<uart_port_t>(CONFIG_GPS_UART_NUMBER), CONFIG_GPS_UART_TXD, CONFIG_GPS_UART_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
        ESP_LOGE(TAG, "config uart gpio failed");
        ReleaseResources();
        return;
    }

    /* Set pattern interrupt, used to detect the end of a line */
    uart_enable_pattern_det_baud_intr(static_cast<uart_port_t>(CONFIG_GPS_UART_NUMBER), '\n', 1, 9, 0, 0);

    /* Set pattern queue size */
    uart_pattern_queue_reset(static_cast<uart_port_t>(CONFIG_GPS_UART_NUMBER), UART_EVENT_QUEUE_SIZE);

    uart_flush(static_cast<uart_port_t>(CONFIG_GPS_UART_NUMBER));

    BaseType_t err = xTaskCreate(
                         nmeaParserTask,
                         TAG,
                         CONFIG_GPS_LOCAL_TASK_STACK_SIZE,
                         esp_gps,
                         CONFIG_GPS_LOCAL_TASK_PRIORITY,
                         &m_taskHandle);
    if (err != pdTRUE) {
        ESP_LOGE(TAG, "create NMEA Parser task failed");
        ReleaseResources();
        return;
    }

    ESP_LOGI(TAG, "NMEA Parser init OK");
    _esp_gps = esp_gps;
}

GPS::~GPS()
{
    ReleaseResources();
}

gps_t GPS::get_gps_data()
{
    if (_esp_gps) {
        return _esp_gps->parent;
    }
    
    // Return empty/default GPS data if not initialized
    gps_t empty_gps = {};
    return empty_gps;
}

esp_err_t GPS::register_event_handler(esp_event_handler_t event_handler, void* handler_arg)
{
    if (!_esp_gps || !_esp_gps->event_loop_hdl) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return esp_event_handler_register_with(_esp_gps->event_loop_hdl, GPS_EVENTS, ESP_EVENT_ANY_ID, event_handler, handler_arg);
}

esp_err_t GPS::unregister_event_handler(esp_event_handler_t event_handler)
{
    if (!_esp_gps || !_esp_gps->event_loop_hdl) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return esp_event_handler_unregister_with(_esp_gps->event_loop_hdl, GPS_EVENTS, ESP_EVENT_ANY_ID, event_handler);
}

