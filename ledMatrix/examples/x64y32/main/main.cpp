#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"

static const char *TAG = "x64y32";
MatrixPanel_I2S_DMA *dma_display = nullptr;

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "LED Matrix Test Program");

  HUB75_I2S_CFG mxconfig(/* width = */ 64, /* height = */ 32, /* chain = */ 2);

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(255);
  dma_display->clearScreen();

  while (true)
  {
    dma_display->fillScreenRGB888(255, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    dma_display->fillScreenRGB888(128, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    dma_display->fillScreenRGB888(64, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    dma_display->fillScreenRGB888(0, 255, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    dma_display->fillScreenRGB888(0, 128, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    dma_display->fillScreenRGB888(0, 64, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    dma_display->fillScreenRGB888(0, 0, 255);
    vTaskDelay(pdMS_TO_TICKS(1000));
    dma_display->fillScreenRGB888(0, 0, 128);
    vTaskDelay(pdMS_TO_TICKS(1000));
    dma_display->fillScreenRGB888(0, 0, 64);
    vTaskDelay(pdMS_TO_TICKS(1000));
    dma_display->fillRect(32, 5, 64, 10, 0, 255, 0);
    dma_display->fillRect(32, 17, 64, 10, 255, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
