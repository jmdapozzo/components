// #include <stdio.h>
// #include <esp_log.h>
// #include <ledPanel.hpp>

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_idf_version.h>
#include <esp_log.h>
#include <max7219.h>

#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 0, 0)
#define HOST    HSPI_HOST
#else
#define HOST    SPI3_HOST
#endif

#define CASCADE_SIZE 4
// #define MOSI_PIN 23
// #define CS_PIN 5
// #define CLK_PIN 18

static const char *TAG = "main";

static const uint64_t symbols[] = {

    0x3c66666e76663c00, // digits 0-9
    0x7e1818181c181800, 
    0x7e060c3060663c00,
    0x3c66603860663c00,
    0x30307e3234383000,
    0x3c6660603e067e00,
    0x3c66663e06663c00,
    0x1818183030667e00,
    0x3c66663c66663c00,
    0x3c66607c66663c00 ,

    0x383838fe7c381000, // arrow up
    0x10387cfe38383800, //arrow down
    0x10307efe7e301000, //arrow right
    0x1018fcfefc181000 //arrow left
    
};
static const size_t symbols_size = sizeof(symbols) - sizeof(uint64_t) * CASCADE_SIZE;

void task(void *pvParameter)
{

    spi_bus_config_t cfg = {
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
    ESP_ERROR_CHECK(spi_bus_initialize(HOST, &cfg, SPI_DMA_CH_AUTO));


    max7219_t dev = {
       .digits = 0,
       .cascade_size = CASCADE_SIZE,
       .mirrored = true
    };
    ESP_ERROR_CHECK(max7219_init_desc(&dev, HOST, MAX7219_MAX_CLOCK_SPEED_HZ, static_cast<gpio_num_t>(CONFIG_LED_PANEL_LATCH)));
    ESP_ERROR_CHECK(max7219_init(&dev));

    size_t offs = 0;

   while (1)
    {
        for (uint8_t i=0; i<CASCADE_SIZE; i++)
        max7219_draw_image_8x8(&dev,i*8,(uint8_t *)symbols + i*8 + offs);
        vTaskDelay(pdMS_TO_TICKS(500));

        if (++offs == symbols_size)
            offs = 0;
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "LED Panel Test Program");

    // const lvgl_port_cfg_t lvglConfig = ESP_LVGL_PORT_INIT_CONFIG();
    // lvgl_port_init(&lvglConfig);

    // macdap::LedPanel ledPanel(macdap::w16h8, 3, 2);
    // ledPanel.setBrightness(1.00);
    // ledPanel.scrollingMessage("Hello World from MacDap");

    xTaskCreatePinnedToCore(task, "task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL, APP_CPU_NUM);

    while (true)
    {
        ESP_LOGI(TAG, "LED Panel Test Program");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
