#include <ledMatrix.hpp>
#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"

using namespace macdap;

static const char *TAG = "ledMatrix";

static void flushCB(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t *color_p)
{

    MatrixPanel_I2S_DMA *display = static_cast<MatrixPanel_I2S_DMA*>(disp_drv->user_data);
    for(int32_t y = area->y1; y <= area->y2; y++) {
        int32_t virtualY = y % CONFIG_LED_MATRIX_PIXEL_HEIGHT;
        for(int32_t x = area->x1; x <= area->x2; x++) {
            int32_t virtualX = (x % disp_drv->hor_res) + ((y / CONFIG_LED_MATRIX_PIXEL_HEIGHT) * disp_drv->hor_res);
            display->drawPixel(virtualX, virtualY, static_cast<uint16_t>(color_p->full));
            color_p++;
        }
    }

    lv_disp_flush_ready(disp_drv);
}

LedMatrix::LedMatrix()
{
    ESP_LOGI(TAG, "Initializing");

    HUB75_I2S_CFG mxconfig;

    MatrixPanel_I2S_DMA *display = new MatrixPanel_I2S_DMA(mxconfig);
    display->begin();
    display->setPanelBrightness(255);
    display->clearScreen();

    m_horizontalResolution = CONFIG_LED_MATRIX_MODULE_WIDTH * CONFIG_LED_MATRIX_PIXEL_WIDTH;
    m_verticalResolution = CONFIG_LED_MATRIX_MODULE_HEIGHT * CONFIG_LED_MATRIX_PIXEL_HEIGHT;

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
    m_displayDriver.user_data = display;
    m_displayDriver.full_refresh = true;
    m_lvDisp = lv_disp_drv_register(&m_displayDriver);

}

LedMatrix::~LedMatrix()
{
}

lv_disp_t *LedMatrix::getLvDisp()
{
    return m_lvDisp;
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
    MatrixPanel_I2S_DMA *display = static_cast<MatrixPanel_I2S_DMA*>(m_displayDriver.user_data);
    display->setPanelBrightness(brightnessByte);
}
