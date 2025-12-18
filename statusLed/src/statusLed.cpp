#include "statusLed.hpp"
#include <esp_log.h>
#include <esp_err.h>
#include <driver/gpio.h>

#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)

using namespace macdap;

static const char *TAG = "statusLed";

static const blink_step_t blink_step_booting[] = { // Solid Green
    {LED_BLINK_HSV, SET_HSV(120, MAX_SATURATION, MAX_BRIGHTNESS), 0},
    {LED_BLINK_STOP, 0, 0},
};

static const blink_step_t blink_step_provisioning[] = { // Green, Blue flashing
    {LED_BLINK_HSV, SET_HSV(120, MAX_SATURATION, MAX_BRIGHTNESS), 500},
    {LED_BLINK_HSV, SET_HSV(240, MAX_SATURATION, MAX_BRIGHTNESS), 500},
    {LED_BLINK_LOOP, 0, 0},
};

static const blink_step_t blink_step_connecting[] = { // Green breathe
    {LED_BLINK_HSV, SET_HSV(120, MAX_SATURATION, 0), 0},
    {LED_BLINK_BREATHE, LED_STATE_ON, 500},
    {LED_BLINK_BREATHE, LED_STATE_OFF, 500},
    {LED_BLINK_LOOP, 0, 0},
};

static const blink_step_t blink_step_running[] = { // Blue breathe
    {LED_BLINK_HSV, SET_HSV(240, MAX_SATURATION, 32), 0},
    {LED_BLINK_BREATHE, LED_STATE_ON, 1000},
    {LED_BLINK_BREATHE, 32, 1000},
    {LED_BLINK_LOOP, 0, 0},
};

static const blink_step_t blink_step_ota_updating[] = { // Yellow breathe
    {LED_BLINK_HSV, SET_HSV(60, MAX_SATURATION, 0), 0},
    {LED_BLINK_BREATHE, LED_STATE_ON, 1000},
    {LED_BLINK_BREATHE, LED_STATE_OFF, 1000},
    {LED_BLINK_LOOP, 0, 0},
};

static const blink_step_t blink_step_error[] = { // Red flash
    {LED_BLINK_HSV, SET_HSV(0, MAX_SATURATION, MAX_BRIGHTNESS), 500},
    {LED_BLINK_HOLD, LED_STATE_OFF, 500},
    {LED_BLINK_LOOP, 0, 0},
};

blink_step_t const *blink_step_list[] = {
    [static_cast<int>(Status::Booting)] = blink_step_booting,
    [static_cast<int>(Status::Provisioning)] = blink_step_provisioning,
    [static_cast<int>(Status::Connecting)] = blink_step_connecting,
    [static_cast<int>(Status::Running)] = blink_step_running,
    [static_cast<int>(Status::OtaUpdating)] = blink_step_ota_updating,
    [static_cast<int>(Status::Error)] = blink_step_error,
    [static_cast<int>(Status::Count)] = nullptr
};

StatusLed::StatusLed()
{
    ESP_LOGI(TAG, "Initializing...");

    led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_STATUS_LED_GPIO,
        .max_leds = CONFIG_STATUS_LED_GPIO_COUNT,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false,
        }};

    #ifdef CONFIG_STATUS_LED_RMT_DRIVER

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = LED_STRIP_RMT_RES_HZ,
        .mem_block_symbols = 64,
        .flags = {
            .with_dma = true,
        }
    };

    led_indicator_strips_config_t strips_config = {
        .led_strip_cfg = strip_config,
        .led_strip_driver = LED_STRIP_RMT,
        .led_strip_rmt_cfg = rmt_config,
    };

    #elif CONFIG_STATUS_LED_SPI_DRIVER

    led_strip_spi_config_t spi_config = {
        .clk_src = SPI_CLK_SRC_DEFAULT,
        .spi_bus = SPI2_HOST,
        .flags = {
            .with_dma = true,
        }};

    led_indicator_strips_config_t strips_config = {
        .led_strip_cfg = strip_config,
        .led_strip_driver = LED_STRIP_SPI,
        .led_strip_spi_cfg = spi_config,
    };

    #endif

    const led_indicator_config_t config = {
        .blink_lists = blink_step_list,
        .blink_list_num = static_cast<int>(macdap::Status::Count)
    };

    ESP_ERROR_CHECK(led_indicator_new_strips_device(&config, &strips_config, &m_led_indicator_handle));
    //ESP_ERROR_CHECK(led_indicator_set_brightness(m_led_indicator_handle, static_cast<uint32_t>(m_brightness)));
}

StatusLed::~StatusLed()
{
}

void StatusLed::clear(Status status)
{
    ESP_ERROR_CHECK(led_indicator_preempt_stop(m_led_indicator_handle, static_cast<int>(status)));
}

void StatusLed::set_on_off(bool on_off)
{
    ESP_ERROR_CHECK(led_indicator_set_on_off(m_led_indicator_handle, on_off));
}

void StatusLed::set_brightness(int8_t brightness)
{
    ESP_ERROR_CHECK(led_indicator_set_brightness(m_led_indicator_handle, static_cast<uint32_t>(brightness)));
}

void StatusLed::set_status(Status status)
{
    ESP_ERROR_CHECK(led_indicator_preempt_start(m_led_indicator_handle, static_cast<int>(status)));
}
