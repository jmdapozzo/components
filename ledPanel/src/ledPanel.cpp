#include <ledPanel.hpp>
#include <string.h>
#include <cmath>
#include <esp_log.h>
#include <esp_check.h>
#include <driver/ledc.h>
// #include <esp_timer.h>
#include "esp_heap_caps.h"

// #include <string.h>
// #include <esp_timer.h>
// #include <esp_check.h>

using namespace macdap;

#ifdef CONFIG_LED_PANEL_INTERFACE_GPIO
#define LOW 0
#define HIGH 1
static const char *TAG = "ledPanel (GPIO)";
#elif CONFIG_LED_PANEL_INTERFACE_SPI
#define SPI_QUEUE_SIZE 10
static const char *TAG = "ledPanel (SPI)";
#endif

#ifdef CONFIG_LED_PANEL_TYPE_MAX7219
#define ALL_DIGITS       8
#define ALL_BITS         8
#define MAX_INTENSITY    15
#define REG_DIGIT_0      (0x01)
#define REG_DECODE_MODE  (0x09)
#define REG_INTENSITY    (0x0A)
#define REG_SCAN_LIMIT   (0x0B)
#define REG_SHUTDOWN     (0x0C)
#define REG_DISPLAY_TEST (0x0F)
#endif

#define PIXEL_PER_BYTE 8

#if LV_COLOR_DEPTH == 32
#define BYTES_PER_PIXEL 4
#elif LV_COLOR_DEPTH == 16
#define BYTES_PER_PIXEL 2
#elif LV_COLOR_DEPTH == 8
#define BYTES_PER_PIXEL 1
#else
#error "Unsupported LV_COLOR_DEPTH"
#endif

static SemaphoreHandle_t _panel_buffer_mutex;

static void set_pixel(LedPanel* ledPanel, int32_t horizontal_resolution, int32_t vertical_resolution, int32_t x, int32_t y, bool state)
{
    if (x < 0 || x >= horizontal_resolution || y < 0 || y >= vertical_resolution)
    {
        ESP_LOGE(TAG, "set_pixel x=%ld, y=%ld out of bounds", x, y);
        return;
    }

    int16_t x_reverse = horizontal_resolution - 1 - x;
    int16_t y_reverse = vertical_resolution - 1 - y;

    uint16_t buffer_index = (y_reverse / PIXEL_PER_BYTE) * horizontal_resolution + x_reverse;
    uint8_t segment = static_cast<uint8_t>(0x80 >> (y_reverse % PIXEL_PER_BYTE));
    uint8_t buffer = ledPanel->get_buffer(buffer_index);

    if (state)
    {
        buffer = buffer | segment;
    }
    else
    {
        buffer = buffer & ~segment;
    }
    ledPanel->set_buffer(buffer_index, buffer);
}

static void flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
#if BYTES_PER_PIXEL == 1
    uint8_t *local_px_map = px_map;
#elif BYTES_PER_PIXEL == 2
    uint16_t *local_px_map = (uint16_t *)px_map;
#elif BYTES_PER_PIXEL == 4
    uint32_t *local_px_map = (uint32_t *)px_map;
#else
#error "LV_COLOR_DEPTH should be 1, 8, 16 or 32"
#endif

    LedPanel* ledPanel = static_cast<LedPanel*>(lv_display_get_user_data(display));

    xSemaphoreTake(_panel_buffer_mutex, portMAX_DELAY);

    int32_t horizontal_resolution = lv_display_get_horizontal_resolution(display);
    int32_t vertical_resolution = lv_display_get_vertical_resolution(display);

    int32_t x, y;
    for(y = area->y1; y <= area->y2; y++) {
        for(x = area->x1; x <= area->x2; x++) {
            set_pixel(ledPanel, horizontal_resolution, vertical_resolution, x, y, *local_px_map != 0);
            local_px_map++;
        }
    }

    ledPanel->send_buffer();

    lv_disp_flush_ready(display);

    xSemaphoreGive(_panel_buffer_mutex);
}

LedPanel::LedPanel()
{
    ESP_LOGI(TAG, "Initializing");

    _panel_buffer_mutex = xSemaphoreCreateMutex();
    if (_panel_buffer_mutex == nullptr)
    {
        ESP_LOGE(TAG, "Create panel_buffer mutex failure!");
        return;
    }

#ifdef CONFIG_LED_PANEL_INTERFACE_GPIO
    gpio_reset_pin(static_cast<gpio_num_t>(CONFIG_LED_PANEL_DATA));
    gpio_set_direction(static_cast<gpio_num_t>(CONFIG_LED_PANEL_DATA), GPIO_MODE_OUTPUT);
    gpio_set_level(static_cast<gpio_num_t>(CONFIG_LED_PANEL_DATA), LOW);
    gpio_reset_pin(static_cast<gpio_num_t>(CONFIG_LED_PANEL_CLOCK));
    gpio_set_direction(static_cast<gpio_num_t>(CONFIG_LED_PANEL_CLOCK), GPIO_MODE_OUTPUT);
    gpio_set_level(static_cast<gpio_num_t>(CONFIG_LED_PANEL_CLOCK), LOW);
#endif
#ifdef CONFIG_LED_PANEL_INTERFACE_SPI
    spi_bus_config_t spi_bus_config = {
        .mosi_io_num = CONFIG_LED_PANEL_DATA,
        .miso_io_num = -1,
        .sclk_io_num = CONFIG_LED_PANEL_CLOCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .data_io_default_level = 0,
        .max_transfer_sz = SPI_MAX_DMA_LEN,
        .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS | SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MOSI,
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
        .intr_flags = 0
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &spi_bus_config, SPI_DMA_CH_AUTO));
#endif

#ifdef CONFIG_LED_PANEL_INTERFACE_GPIO
    gpio_reset_pin(static_cast<gpio_num_t>(CONFIG_LED_PANEL_LATCH));
    gpio_set_direction(static_cast<gpio_num_t>(CONFIG_LED_PANEL_LATCH), GPIO_MODE_OUTPUT);
    gpio_set_level(static_cast<gpio_num_t>(CONFIG_LED_PANEL_LATCH), LOW);
#endif
#ifdef CONFIG_LED_PANEL_INTERFACE_SPI
    spi_device_interface_config_t spi_device_interface_config = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .duty_cycle_pos = 0,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .clock_speed_hz = CONFIG_LED_PANEL_INTERFACE_SPI_CLOCK_SPEED,
        .input_delay_ns = 0,
        .sample_point = SPI_SAMPLING_POINT_PHASE_0,
        .spics_io_num = CONFIG_LED_PANEL_LATCH,
        .flags = SPI_DEVICE_NO_DUMMY,
        .queue_size = SPI_QUEUE_SIZE,
        .pre_cb = nullptr,
        .post_cb = nullptr
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI3_HOST, &spi_device_interface_config, &m_spi));
#endif

    int32_t horizontal_resolution = CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MATRIX_WIDTH;
    int32_t vertical_resolution = CONFIG_LED_PANEL_MODULE_HEIGHT * CONFIG_LED_PANEL_MATRIX_HEIGHT;
    ESP_LOGI(TAG, "Display resolution: %ld x %ld", horizontal_resolution, vertical_resolution);

    m_panel_buffer_size = (vertical_resolution / PIXEL_PER_BYTE) * horizontal_resolution * sizeof(uint8_t);
    m_panel_buffer = static_cast<uint8_t*>(heap_caps_malloc(m_panel_buffer_size, MALLOC_CAP_DMA));
    if (m_panel_buffer == nullptr)
    {
        ESP_LOGE(TAG, "Failed to allocate m_panel_buffer on the heap!");
        return;
    }

    size_t lv_buffer_size = horizontal_resolution * vertical_resolution * BYTES_PER_PIXEL;
    ESP_LOGI(TAG, "Allocating lv_buffer of size %zu", lv_buffer_size);
    uint8_t *lv_buffer = static_cast<uint8_t*>(heap_caps_malloc(lv_buffer_size, MALLOC_CAP_DEFAULT));
    if (lv_buffer == nullptr)
    {
        ESP_LOGE(TAG, "Failed to allocate lv_buffer on the heap!");
        return;
    }

    m_display = lv_display_create(horizontal_resolution, vertical_resolution);
    lv_display_set_flush_cb(m_display, flush_cb);
    lv_display_set_user_data(m_display, this);
    lv_display_set_buffers(m_display, lv_buffer, NULL, lv_buffer_size, LV_DISPLAY_RENDER_MODE_FULL);

#ifdef CONFIG_LED_PANEL_TYPE_MBI5026
    ledc_timer_config_t ledc_timer_conf = {
        .speed_mode = static_cast<ledc_mode_t>(CONFIG_LED_PANEL_LEDC_MODE),
        .duty_resolution = static_cast<ledc_timer_bit_t>(CONFIG_LED_PANEL_LEDC_DUTY_RES),
        .timer_num = static_cast<ledc_timer_t>(CONFIG_LED_PANEL_LEDC_TIMER),
        .freq_hz = CONFIG_LED_PANEL_LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = 0
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer_conf));

    ledc_channel_config_t ledc_channel_conf = {
        .gpio_num = CONFIG_LED_PANEL_EN,
        .speed_mode = static_cast<ledc_mode_t>(CONFIG_LED_PANEL_LEDC_MODE),
        .channel = static_cast<ledc_channel_t>(CONFIG_LED_PANEL_LEDC_CHANNEL),
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = static_cast<ledc_timer_t>(CONFIG_LED_PANEL_LEDC_TIMER),
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = {
            .output_invert = 1
        }
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_conf));

    ESP_ERROR_CHECK(ledc_set_duty(static_cast<ledc_mode_t>(CONFIG_LED_PANEL_LEDC_MODE), static_cast<ledc_channel_t>(CONFIG_LED_PANEL_LEDC_CHANNEL), CONFIG_LED_PANEL_INITIAL_DUTY_CYCLE));
    ESP_ERROR_CHECK(ledc_update_duty(static_cast<ledc_mode_t>(CONFIG_LED_PANEL_LEDC_MODE), static_cast<ledc_channel_t>(CONFIG_LED_PANEL_LEDC_CHANNEL)));

#elif CONFIG_LED_PANEL_TYPE_MAX7219

    xSemaphoreTake(_panel_buffer_mutex, portMAX_DELAY);

    m_max_7219_buffer_len = CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MODULE_HEIGHT * CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB;
    m_max_7219_buffer = static_cast<max_7219_buffer_t*>(heap_caps_malloc(m_max_7219_buffer_len * sizeof(max_7219_buffer_t), MALLOC_CAP_DMA));
    if (m_max_7219_buffer == nullptr)
    {
        ESP_LOGE(TAG, "Failed to allocate m_max_7219_buffer on the heap!");
        xSemaphoreGive(_panel_buffer_mutex);
        return;
    }

    for (int chip = 0; chip < m_max_7219_buffer_len; chip++)
    {
        m_max_7219_buffer[chip].command = REG_SHUTDOWN;
        m_max_7219_buffer[chip].data = 0;
    }
    send_buffer(m_max_7219_buffer, m_max_7219_buffer_len * sizeof(max_7219_buffer_t));

    for (int chip = 0; chip < m_max_7219_buffer_len; chip++)
    {
        m_max_7219_buffer[chip].command = REG_DISPLAY_TEST;
        m_max_7219_buffer[chip].data = 0;
    }
    send_buffer(m_max_7219_buffer, m_max_7219_buffer_len * sizeof(max_7219_buffer_t));

    for (int chip = 0; chip < m_max_7219_buffer_len; chip++)
    {
        m_max_7219_buffer[chip].command = REG_SCAN_LIMIT;
        m_max_7219_buffer[chip].data = ALL_DIGITS - 1;
    }
    send_buffer(m_max_7219_buffer, m_max_7219_buffer_len * sizeof(max_7219_buffer_t));

    for (int chip = 0; chip < m_max_7219_buffer_len; chip++)
    {
        m_max_7219_buffer[chip].command = REG_DECODE_MODE;
        m_max_7219_buffer[chip].data = 0;
    }
    send_buffer(m_max_7219_buffer, m_max_7219_buffer_len * sizeof(max_7219_buffer_t));

    for (int digit = 0; digit < ALL_DIGITS; digit++)
    {
        for (int chip = 0; chip < m_max_7219_buffer_len; chip++)
        {
            m_max_7219_buffer[chip].command = REG_DIGIT_0 + digit;
            m_max_7219_buffer[chip].data = 0;
        }
        send_buffer(m_max_7219_buffer, m_max_7219_buffer_len * sizeof(max_7219_buffer_t));
    }

    for (int chip = 0; chip < m_max_7219_buffer_len; chip++)
    {
        m_max_7219_buffer[chip].command = REG_INTENSITY;
        m_max_7219_buffer[chip].data = 0;
    }
    send_buffer(m_max_7219_buffer, m_max_7219_buffer_len * sizeof(max_7219_buffer_t));

    for (int chip = 0; chip < m_max_7219_buffer_len; chip++)
    {
        m_max_7219_buffer[chip].command = REG_SHUTDOWN;
        m_max_7219_buffer[chip].data = 1;
    }
    send_buffer(m_max_7219_buffer, m_max_7219_buffer_len * sizeof(max_7219_buffer_t));

    heap_caps_free(m_max_7219_buffer);
    xSemaphoreGive(_panel_buffer_mutex);

#endif
}

LedPanel::~LedPanel()
{
#ifdef CONFIG_LED_PANEL_INTERFACE_GPIO
#endif
#ifdef CONFIG_LED_PANEL_INTERFACE_SPI
    vSemaphoreDelete(_panel_buffer_mutex);
    spi_bus_remove_device(m_spi);
    spi_bus_free(SPI3_HOST);
#endif
}

lv_display_t *LedPanel::get_lv_display()
{
    return m_display;
}

uint8_t LedPanel::get_buffer(uint16_t index)
{
    assert(index < m_panel_buffer_size);
    return m_panel_buffer[index];
}

void LedPanel::set_buffer(uint16_t index, uint8_t buffer)
{
    assert(index < m_panel_buffer_size);
    m_panel_buffer[index] = buffer;
}

#ifdef CONFIG_LED_PANEL_INTERFACE_GPIO
static void clk()
{
    gpio_set_level(static_cast<gpio_num_t>(CONFIG_LED_PANEL_CLOCK), HIGH);
    gpio_set_level(static_cast<gpio_num_t>(CONFIG_LED_PANEL_CLOCK), LOW);
}

static void send_data(uint8_t data)
{
    for (int8_t pixel = 0; pixel < PIXEL_PER_BYTE; pixel++)
    {
        gpio_set_level(static_cast<gpio_num_t>(CONFIG_LED_PANEL_DATA), data & 0x80);
        clk();
        data = data << 1;
    }
}

void LedPanel::send_buffer(void *buffer, size_t buffer_size)
{
    uint8_t *current_data = static_cast<uint8_t *>(buffer);

    gpio_set_level(static_cast<gpio_num_t>(CONFIG_LED_PANEL_LATCH), LOW);
    for (int8_t buffer_index = 0; buffer_index < buffer_size; buffer_index++)
    {
        send_data(*current_data);
        current_data++;
    }
    gpio_set_level(static_cast<gpio_num_t>(CONFIG_LED_PANEL_LATCH), HIGH);
}
#endif

#ifdef CONFIG_LED_PANEL_INTERFACE_SPI
void LedPanel::send_buffer(void *buffer, size_t buffer_size)
{
    // ESP_LOG_BUFFER_HEX(TAG, buffer, bufferSize);

    spi_transaction_t spi_transaction = {};
    spi_transaction.length = buffer_size * PIXEL_PER_BYTE;
    spi_transaction.tx_buffer = buffer;
    spi_transaction.rx_buffer = nullptr;
    ESP_ERROR_CHECK(spi_device_transmit(m_spi, &spi_transaction));
}
#endif

void LedPanel::send_buffer()
{
    // ESP_LOG_BUFFER_HEX(TAG, m_panelBuffer, m_panelBufferSize);
    // ESP_LOGI(TAG, "--- %d", CONFIG_LED_PANEL_INTERFACE_SPI_CLOCK_SPEED);

    #ifdef CONFIG_LED_PANEL_TYPE_MBI5026
    send_buffer(m_panel_buffer, m_panel_buffer_size);
#elif CONFIG_LED_PANEL_TYPE_MAX7219

    for (int reg_digit = 0; reg_digit < ALL_DIGITS; reg_digit++)
    {
        // TODO Is this needed here?
        if (m_max_7219_buffer == nullptr)
        {
            ESP_LOGE(TAG, "Failed to allocate m_max_7219_buffer on the heap!");
            return;
        }

        // TODO Fix this as lv_memset_00 is no longer available
        // lv_memset_00(m_max_7219_buffer, m_max_7219_buffer_len * sizeof(Max7219Buffer_t));
        uint8_t *panel_buffer = m_panel_buffer;
        for (int chip = 0; chip < m_max_7219_buffer_len; chip++)
        {
            m_max_7219_buffer[chip].command = REG_DIGIT_0 + reg_digit;
            uint8_t segment = 0x80;
            for (uint8_t x_bit = 0; x_bit < ALL_BITS; x_bit++)
            {
                if (*panel_buffer & (0x80 >> reg_digit))
                {
                    m_max_7219_buffer[chip].data |= segment;
                }
                else
                {
                    m_max_7219_buffer[chip].data &= ~segment;
                }
                segment >>= 1;
                panel_buffer++;
            }
        }
        send_buffer(m_max_7219_buffer, m_max_7219_buffer_len * sizeof(max_7219_buffer_t));
    }

#endif
}

void LedPanel::set_intensity(float intensity)
{
    if (intensity > 1.0)
    {
        intensity = 1.0;
    }
    if (intensity < 0.0)
    {
        intensity = 0.0;
    }

#ifdef CONFIG_LED_PANEL_TYPE_MBI5026
    uint32_t duty_cycle = (intensity * (1 << CONFIG_LED_PANEL_LEDC_DUTY_RES));
    ESP_ERROR_CHECK(ledc_set_duty(static_cast<ledc_mode_t>(CONFIG_LED_PANEL_LEDC_MODE), static_cast<ledc_channel_t>(CONFIG_LED_PANEL_LEDC_CHANNEL), duty_cycle));
    ESP_ERROR_CHECK(ledc_update_duty(static_cast<ledc_mode_t>(CONFIG_LED_PANEL_LEDC_MODE), static_cast<ledc_channel_t>(CONFIG_LED_PANEL_LEDC_CHANNEL)));
#elif CONFIG_LED_PANEL_TYPE_MAX7219
    xSemaphoreTake(_panel_buffer_mutex, portMAX_DELAY);
    uint8_t duty_cycle = static_cast<uint8_t>(roundf(intensity * MAX_INTENSITY));
    for (int chip = 0; chip < m_max_7219_buffer_len; chip++)
    {
        m_max_7219_buffer[chip].command = REG_INTENSITY;
        m_max_7219_buffer[chip].data = duty_cycle;
    }
    send_buffer(m_max_7219_buffer, m_max_7219_buffer_len * sizeof(max_7219_buffer_t));
    xSemaphoreGive(_panel_buffer_mutex);
#endif
}
