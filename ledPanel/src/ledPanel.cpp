#include <ledPanel.hpp>
#include <string.h>
#include <cmath>
#include <esp_log.h>
#include <esp_check.h>
#include <driver/ledc.h>
#include <esp_timer.h>
#include "esp_heap_caps.h"

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

static SemaphoreHandle_t _panelBufferMutex;

static void setPixel(LedPanel* ledPanel, int32_t x, int32_t y, lv_color_t * color)
{
    uint16_t horizontalResolution = ledPanel->getHorizontalResolution();
    uint16_t verticalResolution = ledPanel->getVerticalResolution();

    if (x < 0 || x >= horizontalResolution || y < 0 || y >= verticalResolution)
    {
        ESP_LOGE(TAG, "setPixel x=%ld, y=%ld out of bounds", x, y);
        return;
    }

    int16_t xReverse = horizontalResolution - 1 - x;
    int16_t yReverse = verticalResolution - 1 - y;

    uint16_t bufferIndex = (yReverse / PIXEL_PER_BYTE) * horizontalResolution + xReverse;
    uint8_t segment = static_cast<uint8_t>(0x80 >> (yReverse % PIXEL_PER_BYTE));
    uint8_t buffer = ledPanel->getBuffer(bufferIndex);

    if (color->full == 0)
    {
        buffer = buffer | segment;
    }
    else
    {
        buffer = buffer & ~segment;
    }
    ledPanel->setBuffer(bufferIndex, buffer);
}

static void flushCB(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    LedPanel* ledPanel = static_cast<LedPanel*>(disp_drv->user_data);

    xSemaphoreTake(_panelBufferMutex, portMAX_DELAY);

    lv_color_t color_black = lv_color_black(); 
    lv_color_t color_white = lv_color_white();
    uint16_t horizontalResolution = ledPanel->getHorizontalResolution();
    uint16_t verticalResolution = ledPanel->getVerticalResolution();

    int32_t x, y;
    for(y = area->y1; y <= area->y2; y++) {
        for(x = area->x1; x <= area->x2; x++) {
            setPixel(ledPanel, x, y, color_p);
            color_p++;
        }
    }

    ledPanel->sendBuffer();

    lv_disp_flush_ready(disp_drv);

    xSemaphoreGive(_panelBufferMutex);
}

LedPanel::LedPanel()
{
    ESP_LOGI(TAG, "Initializing");

    _panelBufferMutex = xSemaphoreCreateMutex();
    if (_panelBufferMutex == nullptr)
    {
        ESP_LOGE(TAG, "Create panelBuffer mutex failure!");
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
    spi_bus_config_t spiBusConfig = {
        .mosi_io_num = CONFIG_LED_PANEL_DATA,
        .miso_io_num = -1,
        .sclk_io_num = CONFIG_LED_PANEL_CLOCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SPI_MAX_DMA_LEN,
        .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS | SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MOSI,
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
        .intr_flags = 0
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &spiBusConfig, SPI_DMA_CH_AUTO));
#endif

#ifdef CONFIG_LED_PANEL_INTERFACE_GPIO
    gpio_reset_pin(static_cast<gpio_num_t>(CONFIG_LED_PANEL_LATCH));
    gpio_set_direction(static_cast<gpio_num_t>(CONFIG_LED_PANEL_LATCH), GPIO_MODE_OUTPUT);
    gpio_set_level(static_cast<gpio_num_t>(CONFIG_LED_PANEL_LATCH), LOW);
#endif
#ifdef CONFIG_LED_PANEL_INTERFACE_SPI
    spi_device_interface_config_t spiDeviceInterfaceConfig = {
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
        .spics_io_num = CONFIG_LED_PANEL_LATCH,
        .flags = SPI_DEVICE_NO_DUMMY,
        .queue_size = SPI_QUEUE_SIZE,
        .pre_cb = nullptr,
        .post_cb = nullptr
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI3_HOST, &spiDeviceInterfaceConfig, &m_spi));
#endif

    m_horizontalResolution = CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MATRIX_WIDTH;
    m_verticalResolution = CONFIG_LED_PANEL_MODULE_HEIGHT * CONFIG_LED_PANEL_MATRIX_HEIGHT;

    m_panelBufferSize = (m_verticalResolution / PIXEL_PER_BYTE) * m_horizontalResolution * sizeof(uint8_t);
    m_panelBuffer = static_cast<uint8_t*>(heap_caps_malloc(m_panelBufferSize, MALLOC_CAP_DMA));
    if (m_panelBuffer == nullptr)
    {
        ESP_LOGE(TAG, "Failed to allocate panelBuffer on the heap!");
        return;
    }

    size_t lvBufferSize = m_horizontalResolution * m_verticalResolution * sizeof(lv_color_t);
    lv_color_t *lvBuffer = static_cast<lv_color_t*>(heap_caps_malloc(lvBufferSize, MALLOC_CAP_DEFAULT));
    if (lvBuffer == nullptr)
    {
        ESP_LOGE(TAG, "Failed to allocate lvBuffer on the heap!");
        return;
    }

    lv_disp_draw_buf_init(&m_displayDrawBuffer, lvBuffer, NULL, lvBufferSize);   
    lv_disp_drv_init(&m_displayDriver);
    m_displayDriver.draw_buf = &m_displayDrawBuffer;
    m_displayDriver.hor_res = m_horizontalResolution;
    m_displayDriver.ver_res = m_verticalResolution;
    m_displayDriver.flush_cb = flushCB;
    m_displayDriver.user_data = this;
    m_displayDriver.full_refresh = true;
    m_lvDisp = lv_disp_drv_register(&m_displayDriver);

#ifdef CONFIG_LED_PANEL_TYPE_MBI5026
    ledc_timer_config_t ledc_timer = {
        .speed_mode = static_cast<ledc_mode_t>(CONFIG_LED_PANEL_LEDC_MODE),
        .duty_resolution = static_cast<ledc_timer_bit_t>(CONFIG_LED_PANEL_LEDC_DUTY_RES),
        .timer_num = static_cast<ledc_timer_t>(CONFIG_LED_PANEL_LEDC_TIMER),
        .freq_hz = CONFIG_LED_PANEL_LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = 0
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .gpio_num = CONFIG_LED_PANEL_EN,
        .speed_mode = static_cast<ledc_mode_t>(CONFIG_LED_PANEL_LEDC_MODE),
        .channel = static_cast<ledc_channel_t>(CONFIG_LED_PANEL_LEDC_CHANNEL),
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = static_cast<ledc_timer_t>(CONFIG_LED_PANEL_LEDC_TIMER),
        .duty = 0,
        .hpoint = 0,
        .flags = {
            .output_invert = 1
        }
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ESP_ERROR_CHECK(ledc_set_duty(static_cast<ledc_mode_t>(CONFIG_LED_PANEL_LEDC_MODE), static_cast<ledc_channel_t>(CONFIG_LED_PANEL_LEDC_CHANNEL), CONFIG_LED_PANEL_INITIAL_DUTY_CYCLE));
    ESP_ERROR_CHECK(ledc_update_duty(static_cast<ledc_mode_t>(CONFIG_LED_PANEL_LEDC_MODE), static_cast<ledc_channel_t>(CONFIG_LED_PANEL_LEDC_CHANNEL)));

#elif CONFIG_LED_PANEL_TYPE_MAX7219

    xSemaphoreTake(_panelBufferMutex, portMAX_DELAY);

    m_max7219BufferLen = CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MODULE_HEIGHT * CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB;
    m_max7219Buffer = static_cast<Max7219Buffer_t*>(heap_caps_malloc(m_max7219BufferLen * sizeof(Max7219Buffer_t), MALLOC_CAP_DMA));
    if (m_max7219Buffer == nullptr)
    {
        ESP_LOGE(TAG, "Failed to allocate m_max7219Buffer on the heap!");
        xSemaphoreGive(_panelBufferMutex);
        return;
    }

    for (int chip = 0; chip < m_max7219BufferLen; chip++)
    {
        m_max7219Buffer[chip].command = REG_SHUTDOWN;
        m_max7219Buffer[chip].data = 0;
    }
    sendBuffer(m_max7219Buffer, m_max7219BufferLen * sizeof(Max7219Buffer_t));

    for (int chip = 0; chip < m_max7219BufferLen; chip++)
    {
        m_max7219Buffer[chip].command = REG_DISPLAY_TEST;
        m_max7219Buffer[chip].data = 0;
    }
    sendBuffer(m_max7219Buffer, m_max7219BufferLen * sizeof(Max7219Buffer_t));

    for (int chip = 0; chip < m_max7219BufferLen; chip++)
    {
        m_max7219Buffer[chip].command = REG_SCAN_LIMIT;
        m_max7219Buffer[chip].data = ALL_DIGITS - 1;
    }
    sendBuffer(m_max7219Buffer, m_max7219BufferLen * sizeof(Max7219Buffer_t));

    for (int chip = 0; chip < m_max7219BufferLen; chip++)
    {
        m_max7219Buffer[chip].command = REG_DECODE_MODE;
        m_max7219Buffer[chip].data = 0;
    }
    sendBuffer(m_max7219Buffer, m_max7219BufferLen * sizeof(Max7219Buffer_t));

    for (int digit = 0; digit < ALL_DIGITS; digit++)
    {
        for (int chip = 0; chip < m_max7219BufferLen; chip++)
        {
            m_max7219Buffer[chip].command = REG_DIGIT_0 + digit;
            m_max7219Buffer[chip].data = 0;
        }
        sendBuffer(m_max7219Buffer, m_max7219BufferLen * sizeof(Max7219Buffer_t));
    }

    for (int chip = 0; chip < m_max7219BufferLen; chip++)
    {
        m_max7219Buffer[chip].command = REG_INTENSITY;
        m_max7219Buffer[chip].data = 0;
    }
    sendBuffer(m_max7219Buffer, m_max7219BufferLen * sizeof(Max7219Buffer_t));

    for (int chip = 0; chip < m_max7219BufferLen; chip++)
    {
        m_max7219Buffer[chip].command = REG_SHUTDOWN;
        m_max7219Buffer[chip].data = 1;
    }
    sendBuffer(m_max7219Buffer, m_max7219BufferLen * sizeof(Max7219Buffer_t));

    heap_caps_free(m_max7219Buffer);
    xSemaphoreGive(_panelBufferMutex);

#endif
}

LedPanel::~LedPanel()
{
#ifdef CONFIG_LED_PANEL_INTERFACE_GPIO
#endif
#ifdef CONFIG_LED_PANEL_INTERFACE_SPI
    vSemaphoreDelete(_panelBufferMutex);
    spi_bus_remove_device(m_spi);
    spi_bus_free(SPI3_HOST);
#endif
}

uint16_t LedPanel::getHorizontalResolution()
{
    return m_horizontalResolution;
}

uint16_t LedPanel::getVerticalResolution()
{
    return m_verticalResolution;
}

uint8_t LedPanel::getBuffer(uint16_t index)
{
    assert(index < m_panelBufferSize);
    return m_panelBuffer[index];
}

void LedPanel::setBuffer(uint16_t index, uint8_t buffer)
{
    assert(index < m_panelBufferSize);
    m_panelBuffer[index] = buffer;
}

#ifdef CONFIG_LED_PANEL_INTERFACE_GPIO
static void clk()
{
    gpio_set_level(static_cast<gpio_num_t>(CONFIG_LED_PANEL_CLOCK), HIGH);
    gpio_set_level(static_cast<gpio_num_t>(CONFIG_LED_PANEL_CLOCK), LOW);
}

static void sendData(uint8_t data)
{
    for (int8_t pixel = 0; pixel < PIXEL_PER_BYTE; pixel++)
    {
        gpio_set_level(static_cast<gpio_num_t>(CONFIG_LED_PANEL_DATA), data & 0x80);
        clk();
        data = data << 1;
    }
}

void LedPanel::sendBuffer(void *buffer, size_t bufferSize)
{
    void *currentData = buffer;

    gpio_set_level(static_cast<gpio_num_t>(CONFIG_LED_PANEL_LATCH), LOW);
    for (int8_t bufferIndex = 0; bufferIndex < bufferSize; bufferIndex++)
    {
        sendData(*currentData);
        currentData++;
    }
    gpio_set_level(static_cast<gpio_num_t>(CONFIG_LED_PANEL_LATCH), HIGH);
}
#endif

#ifdef CONFIG_LED_PANEL_INTERFACE_SPI
void LedPanel::sendBuffer(void *buffer, size_t bufferSize)
{
    // ESP_LOG_BUFFER_HEX(TAG, buffer, bufferSize);

    spi_transaction_t spiTransaction = {
        .flags = 0,
        .cmd = 0,
        .addr = 0,
        .length = bufferSize * PIXEL_PER_BYTE,
        .rxlength = 0,
        .user = nullptr,
        .tx_buffer = buffer,
        .rx_buffer = nullptr
    };
    ESP_ERROR_CHECK(spi_device_transmit(m_spi, &spiTransaction));
}
#endif

void LedPanel::sendBuffer()
{
    // ESP_LOG_BUFFER_HEX(TAG, m_panelBuffer, m_panelBufferSize);
    // ESP_LOGI(TAG, "--- %d", CONFIG_LED_PANEL_INTERFACE_SPI_CLOCK_SPEED);

    #ifdef CONFIG_LED_PANEL_TYPE_MBI5026
    sendBuffer(m_panelBuffer, m_panelBufferSize);
#elif CONFIG_LED_PANEL_TYPE_MAX7219

    for (int regDigit = 0; regDigit < ALL_DIGITS; regDigit++)
    {
        if (m_max7219Buffer == nullptr)
        {
            ESP_LOGE(TAG, "Failed to allocate m_max7219Buffer on the heap!");
            return;
        }
        lv_memset_00(m_max7219Buffer, m_max7219BufferLen * sizeof(Max7219Buffer_t));
        uint8_t *panelBuffer = m_panelBuffer;
        for (int chip = 0; chip < m_max7219BufferLen; chip++)
        {
            m_max7219Buffer[chip].command = REG_DIGIT_0 + regDigit;
            uint8_t segment = 0x80;
            for (uint8_t xBit = 0; xBit < ALL_BITS; xBit++)
            {
                if (*panelBuffer & (0x80 >> regDigit))
                {
                    m_max7219Buffer[chip].data |= segment;
                }
                else
                {
                    m_max7219Buffer[chip].data &= ~segment;
                }
                segment >>= 1;
                panelBuffer++;
            }
        }
        sendBuffer(m_max7219Buffer, m_max7219BufferLen * sizeof(Max7219Buffer_t));
    }

#endif
}

lv_disp_t *LedPanel::getLvDisp()
{
    return m_lvDisp;
}

void LedPanel::setBrightness(float brightness)
{
    if (brightness > 100.0)
    {
        brightness = 100.0;
    }
    if (brightness < 0.0)
    {
        brightness = 0.0;
    }

#ifdef CONFIG_LED_PANEL_TYPE_MBI5026
    uint32_t dutyCycle = ((brightness / 100.0) * (1 << CONFIG_LED_PANEL_LEDC_DUTY_RES));
    ESP_ERROR_CHECK(ledc_set_duty(static_cast<ledc_mode_t>(CONFIG_LED_PANEL_LEDC_MODE), static_cast<ledc_channel_t>(CONFIG_LED_PANEL_LEDC_CHANNEL), dutyCycle));
    ESP_ERROR_CHECK(ledc_update_duty(static_cast<ledc_mode_t>(CONFIG_LED_PANEL_LEDC_MODE), static_cast<ledc_channel_t>(CONFIG_LED_PANEL_LEDC_CHANNEL)));
#elif CONFIG_LED_PANEL_TYPE_MAX7219
    uint8_t dutyCycle = static_cast<uint8_t>(roundf((brightness / 100.0f) * MAX_INTENSITY));
    for (int chip = 0; chip < m_max7219BufferLen; chip++)
    {
        m_max7219Buffer[chip].command = REG_INTENSITY;
        m_max7219Buffer[chip].data = dutyCycle;
    }
    xSemaphoreTake(_panelBufferMutex, portMAX_DELAY);
    sendBuffer(m_max7219Buffer, m_max7219BufferLen * sizeof(Max7219Buffer_t));
    xSemaphoreGive(_panelBufferMutex);
#endif
}
