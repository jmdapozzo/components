#include <ledMatrix.hpp>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

using namespace macdap;

static const char *TAG = "ledMatrix";

static void flushCB(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    uint16_t *pxMap = (uint16_t *)px_map;
    uint32_t horizontalResolution = lv_display_get_horizontal_resolution(display);
    MatrixPanel_I2S_DMA *matrixPanel = static_cast<MatrixPanel_I2S_DMA*>(lv_display_get_user_data(display));
    for(int32_t y = area->y1; y <= area->y2; y++) {
        int32_t virtualY = y % CONFIG_LED_MATRIX_PIXEL_HEIGHT;
        for(int32_t x = area->x1; x <= area->x2; x++) {
            int32_t virtualX = (x % horizontalResolution) + ((y / CONFIG_LED_MATRIX_PIXEL_HEIGHT) * horizontalResolution);
            matrixPanel->drawPixel(virtualX, virtualY, *pxMap);
            pxMap++;
        }
    }

    lv_disp_flush_ready(display);
}

LedMatrix::LedMatrix()
{
    ESP_LOGI(TAG, "Initializing");

    HUB75_I2S_CFG mxconfig;
    mxconfig.mx_width = CONFIG_LED_MATRIX_PIXEL_WIDTH;
    mxconfig.mx_height = CONFIG_LED_MATRIX_PIXEL_HEIGHT;
    mxconfig.chain_length = CONFIG_LED_MATRIX_MODULE_WIDTH * CONFIG_LED_MATRIX_MODULE_HEIGHT;
    mxconfig.gpio = {
          CONFIG_LED_MATRIX_HUB75_R1,
          CONFIG_LED_MATRIX_HUB75_G1,
          CONFIG_LED_MATRIX_HUB75_B1,
          CONFIG_LED_MATRIX_HUB75_R2,
          CONFIG_LED_MATRIX_HUB75_G2,
          CONFIG_LED_MATRIX_HUB75_B2,
          CONFIG_LED_MATRIX_HUB75_ADDRA,
          CONFIG_LED_MATRIX_HUB75_ADDRB,
          CONFIG_LED_MATRIX_HUB75_ADDRC,
          CONFIG_LED_MATRIX_HUB75_ADDRD,
          CONFIG_LED_MATRIX_HUB75_ADDRE,
          CONFIG_LED_MATRIX_HUB75_LAT,
          CONFIG_LED_MATRIX_HUB75_OE,
          CONFIG_LED_MATRIX_HUB75_CLK},
    mxconfig.driver = static_cast<HUB75_I2S_CFG::shift_driver>(HUB75_I2S_CFG::shift_driver::SHIFTREG);
    mxconfig.line_decoder = static_cast<HUB75_I2S_CFG::line_driver>(HUB75_I2S_CFG::line_driver::TYPE138);
    mxconfig.double_buff = false;
    mxconfig.i2sspeed = static_cast<HUB75_I2S_CFG::clk_speed>(HUB75_I2S_CFG::clk_speed::HZ_8M);
    mxconfig.latch_blanking = 2; //CONFIG_LED_MATRIX_LATCH_BLANKING;
    mxconfig.clkphase = true;
    mxconfig.min_refresh_rate = 60; //CONFIG_LED_MATRIX_MIN_REFRESH_RATE;
    mxconfig.setPixelColorDepthBits(6); //CONFIG_LED_MATRIX_COLOR_DEPTH;

    MatrixPanel_I2S_DMA *display = new MatrixPanel_I2S_DMA(mxconfig);
    display->begin();
    display->setPanelBrightness(255);
    display->clearScreen();
    // display->setLatBlanking(4); // See https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA Latch Blanking for more information

    int32_t horizontalResolution = CONFIG_LED_MATRIX_MODULE_WIDTH * CONFIG_LED_MATRIX_PIXEL_WIDTH;
    int32_t verticalResolution = CONFIG_LED_MATRIX_MODULE_HEIGHT * CONFIG_LED_MATRIX_PIXEL_HEIGHT;

    ESP_LOGI(TAG, "Display resolution: %ld x %ld", horizontalResolution, verticalResolution);

    #define BYTES_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))
    size_t lvBufferSize = horizontalResolution * verticalResolution * BYTES_PER_PIXEL;
    uint8_t *lvBuffer = static_cast<uint8_t*>(heap_caps_malloc(lvBufferSize, MALLOC_CAP_DEFAULT));
    if (lvBuffer == nullptr)
    {
        ESP_LOGE(TAG, "Failed to allocate lvBuffer on the heap!");
        return;
    }

    m_display = lv_display_create(horizontalResolution, verticalResolution);
    lv_display_set_flush_cb(m_display, flushCB);
    lv_display_set_user_data(m_display, display);
    lv_display_set_buffers(m_display, lvBuffer, NULL, lvBufferSize, LV_DISPLAY_RENDER_MODE_FULL);
}

LedMatrix::~LedMatrix()
{
}

lv_display_t *LedMatrix::getLvDisplay()
{
    return m_display;
}

void LedMatrix::setBrightness(float brightness)
{
    if (brightness > 100.0)
    {
        brightness = 100.0;
    }
    if (brightness < 0.0)
    {
        brightness = 0.0;
    }
    uint8_t brightnessByte = static_cast<uint8_t>(std::round((brightness / 100.0f) * 255.0f));
    MatrixPanel_I2S_DMA *display = static_cast<MatrixPanel_I2S_DMA*>(lv_display_get_user_data(m_display));
    display->setPanelBrightness(brightnessByte);
}
