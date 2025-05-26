#include <ledPanel.hpp>
#include <logo.h>
#include <font.h>
#include <string.h>
#include <esp_log.h>
#include <esp_check.h>
#include <driver/ledc.h>
#include <esp_timer.h>
#include "esp_heap_caps.h"

using namespace macdap;

#define TIMER_PERIOD_MS 5
#define TASK_MAX_SLEEP_MS 500
#define TASK_STACK_SIZE 6144
#define TASK_PRIORITY 4

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
#define REG_DIGIT_0      (0x01)
#define REG_DECODE_MODE  (0x09)
#define REG_INTENSITY    (0x0A)
#define REG_SCAN_LIMIT   (0x0B)
#define REG_SHUTDOWN     (0x0C)
#define REG_DISPLAY_TEST (0x0F)
#endif

#define PIXEL_PER_BYTE 8

static SemaphoreHandle_t panelBufferMutex;

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

#ifdef CONFIG_LED_PANEL_TYPE_MBI5026
    uint16_t index = (yReverse / PIXEL_PER_BYTE) * horizontalResolution + xReverse;
    uint8_t segment = static_cast<uint8_t>(0x80 >> (yReverse % PIXEL_PER_BYTE));
    panelBuffer_t buffer = ledPanel->getBuffer(index);
#elif CONFIG_LED_PANEL_TYPE_MAX7219
    uint16_t index = ((xReverse / CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB) + (yReverse * PIXEL_PER_BYTE)) / sizeof(panelBuffer_t);
    panelBuffer_t buffer = ledPanel->getBuffer(index);
    buffer.command = REG_DIGIT_0 + (yReverse % PIXEL_PER_BYTE);
    uint8_t segment = static_cast<uint8_t>(0x80 >> (xReverse % PIXEL_PER_BYTE));
#endif

    if (color->full == 0)
    {
        buffer.data = buffer.data | segment;
    }
    else
    {
        buffer.data = buffer.data & ~segment;
    }
    ledPanel->setBuffer(index, buffer);
}

static void flushCB(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    LedPanel* ledPanel = static_cast<LedPanel*>(disp_drv->user_data);

    xSemaphoreTake(panelBufferMutex, portMAX_DELAY);

    int32_t x, y;
    for(y = area->y1; y <= area->y2; y++) {
        for(x = area->x1; x <= area->x2; x++) {
            setPixel(ledPanel, x, y, color_p);
            color_p++;
        }
    }
    
    ledPanel->sendBuffer();

    lv_disp_flush_ready(disp_drv);

    xSemaphoreGive(panelBufferMutex);
}

LedPanel::LedPanel()
{
    ESP_LOGI(TAG, "Initializing");

    panelBufferMutex = xSemaphoreCreateMutex();
    if (panelBufferMutex == nullptr)
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
        .clock_speed_hz = 2000000, // SPI_MASTER_FREQ_8M; //SPI_MASTER_FREQ_20M;
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

    m_panelBufferSize = (m_verticalResolution / PIXEL_PER_BYTE) * m_horizontalResolution;
    m_panelBuffer = static_cast<panelBuffer_t*>(heap_caps_malloc(m_panelBufferSize * sizeof(panelBuffer_t), MALLOC_CAP_DMA));
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

    xSemaphoreTake(panelBufferMutex, portMAX_DELAY);

    for (int i = 0; i < CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB; i++)
    {
        m_panelBuffer[i].command = REG_SHUTDOWN;
        m_panelBuffer[i].data = 0;
    }
    sendBuffer(m_panelBuffer, CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB);

    for (int i = 0; i < CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB; i++)
    {
        m_panelBuffer[i].command = REG_DISPLAY_TEST;
        m_panelBuffer[i].data = 0;
    }
    sendBuffer(m_panelBuffer, CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB);

    for (int i = 0; i < CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB; i++)
    {
        m_panelBuffer[i].command = REG_SCAN_LIMIT;
        m_panelBuffer[i].data = ALL_DIGITS - 1;
    }
    sendBuffer(m_panelBuffer, CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB);

    for (int i = 0; i < CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB; i++)
    {
        m_panelBuffer[i].command = REG_DECODE_MODE;
        m_panelBuffer[i].data = 0;
    }
    sendBuffer(m_panelBuffer, CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB);

    for (int j = 0; j < 8; j++)
    {
        for (int i = 0; i < CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB; i++)
        {
            m_panelBuffer[i].command = REG_DIGIT_0 + j;
            m_panelBuffer[i].data = 0;
        }
        sendBuffer(m_panelBuffer, CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB);
    }

    for (int i = 0; i < CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB; i++)
    {
        m_panelBuffer[i].command = REG_INTENSITY;
        m_panelBuffer[i].data = 0;
    }
    sendBuffer(m_panelBuffer, CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB);

    vTaskDelay(pdMS_TO_TICKS(1000));

    for (int i = 0; i < CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB; i++)
    {
        m_panelBuffer[i].command = REG_SHUTDOWN;
        m_panelBuffer[i].data = 1;
    }
    sendBuffer(m_panelBuffer, CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB);

    xSemaphoreGive(panelBufferMutex);

#endif
}

LedPanel::~LedPanel()
{
#ifdef CONFIG_LED_PANEL_INTERFACE_GPIO
#endif
#ifdef CONFIG_LED_PANEL_INTERFACE_SPI
    vSemaphoreDelete(panelBufferMutex);
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

panelBuffer_t LedPanel::getBuffer(uint16_t index)
{
    assert(index < m_panelBufferSize);
    return m_panelBuffer[index];
}

void LedPanel::setBuffer(uint16_t index, panelBuffer_t buffer)
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
    for (int8_t i = 0; i < PIXEL_PER_BYTE * sizeof(panelBuffer_t); i++)
    {
        gpio_set_level(static_cast<gpio_num_t>(CONFIG_LED_PANEL_DATA), data & 0x80);
        clk();
        data = data << 1;
    }
}

void LedPanel::sendBuffer(panelBuffer_t* panelBuffer, size_t panelBufferSize)
{
    uint8_t *currentData = panelBuffer;

    gpio_set_level(static_cast<gpio_num_t>(CONFIG_LED_PANEL_LATCH), LOW);
    for (int8_t i = 0; i < panelBufferSize; i++)
    {
        sendData(*currentData);
        currentData++;
    }
    gpio_set_level(static_cast<gpio_num_t>(CONFIG_LED_PANEL_LATCH), HIGH);
}
#endif

#ifdef CONFIG_LED_PANEL_INTERFACE_SPI
void LedPanel::sendBuffer(panelBuffer_t* panelBuffer, size_t panelBufferSize)
{
    ESP_LOG_BUFFER_HEX(TAG, panelBuffer, panelBufferSize * sizeof(panelBuffer_t));

    spi_transaction_t spiTransaction = {
        .flags = 0,
        .cmd = 0,
        .addr = 0,
        .length = panelBufferSize * PIXEL_PER_BYTE * sizeof(panelBuffer_t),
        .rxlength = 0,
        .user = nullptr,
        .tx_buffer = panelBuffer,
        .rx_buffer = nullptr
    };
    ESP_ERROR_CHECK(spi_device_transmit(m_spi, &spiTransaction));
}
#endif

void LedPanel::sendBuffer()
{
#ifdef CONFIG_LED_PANEL_TYPE_MBI5026
    sendBuffer(m_panelBuffer, m_panelBufferSize);
#elif CONFIG_LED_PANEL_TYPE_MAX7219
    uint16_t bufferIndex = 0;
    for (uint16_t digit = 0; digit < ALL_DIGITS; digit++)
    {
        sendBuffer(&m_panelBuffer[bufferIndex], CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB);
        bufferIndex += CONFIG_LED_PANEL_MODULE_WIDTH * CONFIG_LED_PANEL_MAX7219_MODULE_CHIP_NB;
    };
#endif
}

lv_disp_t *LedPanel::getLvDisp()
{
    return m_lvDisp;
}

void LedPanel::seize()
{
    lvgl_port_lock(0);
}

void LedPanel::release()
{
    lvgl_port_unlock();
}

void LedPanel::clear()
{
    if (m_lvDisp && lvgl_port_lock(0)) 
    {
        lv_obj_t *scr = lv_disp_get_scr_act(m_lvDisp);
        lv_obj_clean(scr);
        lvgl_port_unlock();
    }
}

void LedPanel::greeting(const char *projectName, const char *version)
{
    static lv_obj_t *scr;
    static lv_obj_t *logo;
    static lv_obj_t *labelProjectName;

    if (m_lvDisp && lvgl_port_lock(0)) 
    {
        scr = lv_disp_get_scr_act(m_lvDisp);
        lv_obj_set_style_text_font(scr, &lv_font_unscii_8, 0);
    
        logo = lv_img_create(scr);
        lv_img_set_src(logo, &logo16x16);
        lv_obj_align(logo, LV_ALIGN_LEFT_MID, 0, 0);
    
        lvgl_port_unlock();
    
        vTaskDelay(pdMS_TO_TICKS(3000));

        lvgl_port_lock(0);

        lv_obj_clean(scr);

        labelProjectName = lv_label_create(scr);
        lv_label_set_text(labelProjectName, projectName);
        lv_obj_align(labelProjectName, LV_ALIGN_TOP_LEFT, 0, 0);

        lv_obj_t * labelVersion = lv_label_create(scr);
        lv_label_set_text(labelVersion, version);
        lv_obj_align(labelVersion, LV_ALIGN_BOTTOM_LEFT, 0, 0);

        lvgl_port_unlock();
    
        vTaskDelay(pdMS_TO_TICKS(5000));
    
        // lv_obj_t * label1 = lv_label_create(scr);
        // lv_label_set_text(label1, LV_SYMBOL_OK " Hello");
    
        // lv_obj_t *labelTop = lv_label_create(scr);
        // lv_label_set_long_mode(labelTop, LV_LABEL_LONG_SCROLL_CIRCULAR); /* Circular scroll */
        // lv_label_set_text(labelTop, "Hello, World! This is a long text to test the circular scrolling feature.");
        // lv_obj_set_width(labelTop, m_lvDisp->driver->hor_res);
        // lv_obj_align(labelTop, LV_ALIGN_TOP_MID, 0, 0);
    
        // lv_obj_t * imgLogo = lv_img_create(scr);
        // lv_img_set_src(imgLogo, &macdapLogoVertical);
        // lv_obj_align(imgLogo, LV_ALIGN_CENTER, 0, -20);
        // lv_obj_set_size(imgLogo, 64, 64);
    
        // lv_obj_t *labelBottom = lv_label_create(scr);
        // lv_label_set_long_mode(labelBottom, LV_LABEL_LONG_SCROLL_CIRCULAR); /* Circular scroll */
        // lv_label_set_text_fmt(labelBottom, "%s %s", projectName, version);
        // lv_obj_set_width(labelBottom, m_lvDisp->driver->hor_res);
        // lv_obj_align(labelBottom, LV_ALIGN_BOTTOM_MID, 0, 0);

        //unlock();
    }
}

void LedPanel::message(const char *message)
{
    static lv_obj_t *scr = nullptr;
    static lv_obj_t *label = nullptr;

    if (m_lvDisp && lvgl_port_lock(0))
    {
        if (scr == nullptr)
        {
            scr = lv_disp_get_scr_act(m_lvDisp);
            lv_obj_set_style_text_font(scr, &lv_font_unscii_8, 0);
            label = lv_label_create(scr);
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        }
        lv_label_set_text(label, message);
        lvgl_port_unlock();
    }
}

void LedPanel::scrollingMessage(const char *message)
{
    static lv_obj_t *scr = nullptr;
    static lv_obj_t *label = nullptr;
    static lv_style_t style;

    if (m_lvDisp && lvgl_port_lock(0))
    {
        if (scr == nullptr)
        {
            scr = lv_disp_get_scr_act(m_lvDisp);
            lv_style_init(&style);
            lv_style_set_text_font(&style, &lv_font_unscii_8);
            lv_style_set_anim_speed(&style, 10);
            
            //lv_obj_set_style_text_font(scr, &lv_font_unscii_8, 0);
            label = lv_label_create(scr);
            lv_obj_add_style(label, &style, 0);
            lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
            lv_obj_set_width(label, m_horizontalResolution);
            lv_obj_set_height(label, m_verticalResolution);
            lv_label_set_text(label, message);
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        }

        lv_label_set_text(label, message);
        lvgl_port_unlock();
    }
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
    ESP_LOGD(TAG, "Setting brightness to %.2f%% (duty cycle: %ld)", brightness, dutyCycle);
    ESP_ERROR_CHECK(ledc_set_duty(static_cast<ledc_mode_t>(CONFIG_LED_PANEL_LEDC_MODE), static_cast<ledc_channel_t>(CONFIG_LED_PANEL_LEDC_CHANNEL), dutyCycle));
    ESP_ERROR_CHECK(ledc_update_duty(static_cast<ledc_mode_t>(CONFIG_LED_PANEL_LEDC_MODE), static_cast<ledc_channel_t>(CONFIG_LED_PANEL_LEDC_CHANNEL)));
#elif CONFIG_LED_PANEL_TYPE_MAX7219
    ESP_LOGD(TAG, "Setting brightness to %.2f%% Not yet implemented", brightness);
#endif
}


