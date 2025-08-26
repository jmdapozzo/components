#include "statusLed.hpp"
#include <esp_log.h>
#include <esp_err.h>
#include <driver/gpio.h>

using namespace macdap;

static const char *TAG = "statusLed";

#ifdef CONFIG_STATUS_LED_TYPE_COLOR

static led_strip_handle_t configure_led(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_STATUS_LED_GPIO,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_RGB,
        .flags = {
            .invert_out = false,
        }};

    led_strip_handle_t led_strip;

#ifdef CONFIG_STATUS_LED_RMT_DRIVER

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags = {
            .with_dma = true,
        }
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with RMT backend");

#elif CONFIG_STATUS_LED_SPI_DRIVER

    led_strip_spi_config_t spi_config = {
        .clk_src = SPI_CLK_SRC_DEFAULT,
        .spi_bus = SPI2_HOST,
        .flags = {
            .with_dma = true,
        }};

    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with SPI backend");

#endif

    return led_strip;
}
#endif

StatusLed::StatusLed()
{
    ESP_LOGI(TAG, "Initializing...");

#ifdef CONFIG_STATUS_LED_TYPE_MONOCHROME
    gpio_reset_pin(static_cast<gpio_num_t>(CONFIG_STATUS_LED_GPIO));
    gpio_set_direction(static_cast<gpio_num_t>(CONFIG_STATUS_LED_GPIO), GPIO_MODE_OUTPUT);
#elif CONFIG_STATUS_LED_TYPE_COLOR
    m_ledStrip = configure_led();
    clear();
#endif
}

StatusLed::~StatusLed()
{
}

void StatusLed::clear()
{
#ifdef CONFIG_STATUS_LED_TYPE_MONOCHROME
    gpio_set_level(static_cast<gpio_num_t>(CONFIG_STATUS_LED_GPIO), 0);
#elif CONFIG_STATUS_LED_TYPE_COLOR
    ESP_ERROR_CHECK(led_strip_clear(m_ledStrip));
#endif

    status = Status::Off;
}

void StatusLed::setStatus(Status status)
{
#ifdef CONFIG_STATUS_LED_TYPE_MONOCHROME
    gpio_set_level(static_cast<gpio_num_t>(CONFIG_STATUS_LED_GPIO), (status == Status::Off) || (status == Status::RunningPhase0) ? 0 : 1);
#elif CONFIG_STATUS_LED_TYPE_COLOR
    switch (status)
    {
    case Status::Off:
        ESP_ERROR_CHECK(led_strip_set_pixel(m_ledStrip, 0, 0, 0, 0));
        break;
    case Status::Booting:
        ESP_ERROR_CHECK(led_strip_set_pixel(m_ledStrip, 0, 0, 0, brightness));
        break;
    case Status::Commissioning:
        ESP_ERROR_CHECK(led_strip_set_pixel(m_ledStrip, 0, brightness, 0, 0));
        break;
    case Status::RunningPhase0:
        ESP_ERROR_CHECK(led_strip_set_pixel(m_ledStrip, 0, 0, 0, 0));
        break;
    case Status::RunningPhase1:
        ESP_ERROR_CHECK(led_strip_set_pixel(m_ledStrip, 0, 0, brightness, 0));
        break;
    case Status::Error:
        ESP_ERROR_CHECK(led_strip_set_pixel(m_ledStrip, 0, brightness, 0, 0));
        break;
    }
    ESP_ERROR_CHECK(led_strip_refresh(m_ledStrip));
#endif
}
