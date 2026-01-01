#include <ledMatrix.hpp>
#include "hub75.h"
#include <esp_log.h>

using namespace macdap;

static const char *TAG = "ledMatrix";

static void flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    Hub75Driver *driver = static_cast<Hub75Driver*>(lv_display_get_user_data(display));
 
    const uint16_t x = area->x1;
    const uint16_t y = area->y1;
    const uint16_t w = area->x2 - area->x1 + 1;
    const uint16_t h = area->y2 - area->y1 + 1;

    driver->draw_pixels(x, y, w, h, px_map, Hub75PixelFormat::RGB565);

    #ifdef CONFIG_HUB75_DOUBLE_BUFFER
    driver->flip_buffer();
    #endif

    lv_disp_flush_ready(display);
}

LedMatrix::LedMatrix()
{
    ESP_LOGI(TAG, "Initializing");

    Hub75Config config{};

    // Panel dimensions
    config.panel_width = CONFIG_HUB75_PANEL_WIDTH;
    config.panel_height = CONFIG_HUB75_PANEL_HEIGHT;

    // Scan pattern
#if defined(CONFIG_HUB75_SCAN_1_8)
    config.scan_pattern = Hub75ScanPattern::SCAN_1_8;
#elif defined(CONFIG_HUB75_SCAN_1_16)
    config.scan_pattern = Hub75ScanPattern::SCAN_1_16;
#elif defined(CONFIG_HUB75_SCAN_1_32)
    config.scan_pattern = Hub75ScanPattern::SCAN_1_32;
#endif

    // Scan wiring
#if defined(CONFIG_HUB75_WIRING_STANDARD)
    config.scan_wiring = Hub75ScanWiring::STANDARD_TWO_SCAN;
#elif defined(CONFIG_HUB75_WIRING_FOUR_SCAN_16PX)
    config.scan_wiring = Hub75ScanWiring::FOUR_SCAN_16PX_HIGH;
#elif defined(CONFIG_HUB75_WIRING_FOUR_SCAN_32PX)
    config.scan_wiring = Hub75ScanWiring::FOUR_SCAN_32PX_HIGH;
#elif defined(CONFIG_HUB75_WIRING_FOUR_SCAN_64PX)
    config.scan_wiring = Hub75ScanWiring::FOUR_SCAN_64PX_HIGH;
#endif

    // Shift driver
#if defined(CONFIG_HUB75_DRIVER_GENERIC)
    config.shift_driver = Hub75ShiftDriver::GENERIC;
#elif defined(CONFIG_HUB75_DRIVER_FM6126A)
    config.shift_driver = Hub75ShiftDriver::FM6126A;
#elif defined(CONFIG_HUB75_DRIVER_FM6124)
    config.shift_driver = Hub75ShiftDriver::FM6124;
#elif defined(CONFIG_HUB75_DRIVER_MBI5124)
    config.shift_driver = Hub75ShiftDriver::MBI5124;
#elif defined(CONFIG_HUB75_DRIVER_DP3246)
    config.shift_driver = Hub75ShiftDriver::DP3246;
#endif

    // Pin configuration (board preset or custom)
#if defined(CONFIG_HUB75_BOARD_ADAFRUIT_MATRIX_PORTAL_S3)
    // Adafruit Matrix Portal S3
    config.pins.r1 = 42;
    config.pins.g1 = 41;
    config.pins.b1 = 40;
    config.pins.r2 = 38;
    config.pins.g2 = 39;
    config.pins.b2 = 37;
    config.pins.a = 45;
    config.pins.b = 36;
    config.pins.c = 48;
    config.pins.d = 35;
    config.pins.e = 21;
    config.pins.lat = 47;
    config.pins.oe = 14;
    config.pins.clk = 2;
    ESP_LOGI(TAG, "Board preset: Adafruit Matrix Portal S3");

#elif defined(CONFIG_HUB75_BOARD_APOLLO_M1_REV4)
    // Apollo Automation M1 Rev4 (same pins as Adafruit)
    config.pins.r1 = 42;
    config.pins.g1 = 41;
    config.pins.b1 = 40;
    config.pins.r2 = 38;
    config.pins.g2 = 39;
    config.pins.b2 = 37;
    config.pins.a = 45;
    config.pins.b = 36;
    config.pins.c = 48;
    config.pins.d = 35;
    config.pins.e = 21;
    config.pins.lat = 47;
    config.pins.oe = 14;
    config.pins.clk = 2;
    ESP_LOGI(TAG, "Board preset: Apollo Automation M1 Rev4");

#elif defined(CONFIG_HUB75_BOARD_APOLLO_M1_REV6)
    // Apollo Automation M1 Rev6
    config.pins.r1 = 1;
    config.pins.g1 = 5;
    config.pins.b1 = 6;
    config.pins.r2 = 7;
    config.pins.g2 = 13;
    config.pins.b2 = 9;
    config.pins.a = 16;
    config.pins.b = 48;
    config.pins.c = 47;
    config.pins.d = 21;
    config.pins.e = 38;
    config.pins.lat = 8;
    config.pins.oe = 4;
    config.pins.clk = 18;
    ESP_LOGI(TAG, "Board preset: Apollo Automation M1 Rev6");

#elif defined(CONFIG_HUB75_BOARD_HUIDU_HD_WF2)
    // Huidu HD-WF2
    config.pins.r1 = 2;
    config.pins.g1 = 6;
    config.pins.b1 = 10;
    config.pins.r2 = 3;
    config.pins.g2 = 7;
    config.pins.b2 = 11;
    config.pins.a = 39;
    config.pins.b = 38;
    config.pins.c = 37;
    config.pins.d = 36;
    config.pins.e = 21;  // TODO: Confirm E pin assignment
    config.pins.lat = 33;
    config.pins.oe = 35;
    config.pins.clk = 34;
    ESP_LOGI(TAG, "Board preset: Huidu HD-WF2");

#elif defined(CONFIG_HUB75_BOARD_GENERIC_S3)
    // Generic ESP32-S3 (sequential GPIO 1-14)
    config.pins.r1 = 1;
    config.pins.g1 = 2;
    config.pins.b1 = 3;
    config.pins.r2 = 4;
    config.pins.g2 = 5;
    config.pins.b2 = 6;
    config.pins.a = 7;
    config.pins.b = 8;
    config.pins.c = 9;
    config.pins.d = 10;
    config.pins.e = 11;
    config.pins.lat = 12;
    config.pins.oe = 13;
    config.pins.clk = 14;
    ESP_LOGI(TAG, "Board preset: Generic ESP32-S3");

#else  // CONFIG_HUB75_BOARD_CUSTOM
    // Custom pin configuration from menuconfig
    config.pins.r1 = CONFIG_HUB75_PIN_R1;
    config.pins.g1 = CONFIG_HUB75_PIN_G1;
    config.pins.b1 = CONFIG_HUB75_PIN_B1;
    config.pins.r2 = CONFIG_HUB75_PIN_R2;
    config.pins.g2 = CONFIG_HUB75_PIN_G2;
    config.pins.b2 = CONFIG_HUB75_PIN_B2;
    config.pins.a = CONFIG_HUB75_PIN_A;
    config.pins.b = CONFIG_HUB75_PIN_B;
    config.pins.c = CONFIG_HUB75_PIN_C;
    config.pins.d = CONFIG_HUB75_PIN_D;
    config.pins.e = CONFIG_HUB75_PIN_E;
    config.pins.lat = CONFIG_HUB75_PIN_LAT;
    config.pins.oe = CONFIG_HUB75_PIN_OE;
    config.pins.clk = CONFIG_HUB75_PIN_CLK;
    ESP_LOGI(TAG, "Board preset: Custom");
#endif

    // Multi-panel layout
    config.layout_rows = CONFIG_HUB75_LAYOUT_ROWS;
    config.layout_cols = CONFIG_HUB75_LAYOUT_COLS;

#if defined(CONFIG_HUB75_LAYOUT_HORIZONTAL)
    config.layout = Hub75PanelLayout::HORIZONTAL;
#elif defined(CONFIG_HUB75_LAYOUT_TOP_LEFT_DOWN)
    config.layout = Hub75PanelLayout::TOP_LEFT_DOWN;
#elif defined(CONFIG_HUB75_LAYOUT_TOP_RIGHT_DOWN)
    config.layout = Hub75PanelLayout::TOP_RIGHT_DOWN;
#elif defined(CONFIG_HUB75_LAYOUT_BOTTOM_LEFT_UP)
    config.layout = Hub75PanelLayout::BOTTOM_LEFT_UP;
#elif defined(CONFIG_HUB75_LAYOUT_BOTTOM_RIGHT_UP)
    config.layout = Hub75PanelLayout::BOTTOM_RIGHT_UP;
#elif defined(CONFIG_HUB75_LAYOUT_TOP_LEFT_DOWN_ZIGZAG)
    config.layout = Hub75PanelLayout::TOP_LEFT_DOWN_ZIGZAG;
#elif defined(CONFIG_HUB75_LAYOUT_TOP_RIGHT_DOWN_ZIGZAG)
    config.layout = Hub75PanelLayout::TOP_RIGHT_DOWN_ZIGZAG;
#elif defined(CONFIG_HUB75_LAYOUT_BOTTOM_LEFT_UP_ZIGZAG)
    config.layout = Hub75PanelLayout::BOTTOM_LEFT_UP_ZIGZAG;
#elif defined(CONFIG_HUB75_LAYOUT_BOTTOM_RIGHT_UP_ZIGZAG)
    config.layout = Hub75PanelLayout::BOTTOM_RIGHT_UP_ZIGZAG;
#endif

    // Bit depth & gamma: Configure via menuconfig
    // (idf.py menuconfig → HUB75 → Panel Settings / Color)
    // Or CMake override: -DHUB75_BIT_DEPTH=10 -DHUB75_GAMMA_MODE=0

    // Clock speed
#if defined(CONFIG_HUB75_CLK_8MHZ)
    config.output_clock_speed = Hub75ClockSpeed::HZ_8M;
#elif defined(CONFIG_HUB75_CLK_10MHZ)
    config.output_clock_speed = Hub75ClockSpeed::HZ_10M;
#elif defined(CONFIG_HUB75_CLK_16MHZ)
    config.output_clock_speed = Hub75ClockSpeed::HZ_16M;
#elif defined(CONFIG_HUB75_CLK_20MHZ)
    config.output_clock_speed = Hub75ClockSpeed::HZ_20M;
#endif

    // Performance settings
    config.min_refresh_rate = CONFIG_HUB75_MIN_REFRESH_RATE;
    config.brightness = CONFIG_HUB75_BRIGHTNESS;

    // Timing settings
    config.latch_blanking = CONFIG_HUB75_LATCH_BLANKING;

    // Features (from component Kconfig)
#ifdef CONFIG_HUB75_DOUBLE_BUFFER
    config.double_buffer = true;
#else
    config.double_buffer = false;
#endif

#ifdef CONFIG_HUB75_CLK_PHASE_INVERTED
    config.clk_phase_inverted = true;
#else
    config.clk_phase_inverted = false;
#endif

    // Gamma correction mode
#if defined(CONFIG_HUB75_GAMMA_CIE1931)
    config.gamma_mode = Hub75GammaMode::CIE1931;
#elif defined(CONFIG_HUB75_GAMMA_LINEAR)
    config.gamma_mode = Hub75GammaMode::LINEAR;
#elif defined(CONFIG_HUB75_GAMMA_NONE)
    config.gamma_mode = Hub75GammaMode::LINEAR;  // None is same as Linear
#endif

    // Create and start driver
    Hub75Driver *driver = new Hub75Driver(config);
    driver->begin();
    driver->clear();

    int32_t horizontal_resolution = CONFIG_HUB75_LAYOUT_COLS * CONFIG_HUB75_PANEL_WIDTH;
    int32_t vertical_resolution = CONFIG_HUB75_LAYOUT_ROWS * CONFIG_HUB75_PANEL_HEIGHT;

    ESP_LOGI(TAG, "Display resolution: %ld x %ld", horizontal_resolution, vertical_resolution);

    #define BYTES_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))
    size_t lv_buffer_size = horizontal_resolution * vertical_resolution * BYTES_PER_PIXEL;
    uint8_t *lv_buffer = static_cast<uint8_t*>(heap_caps_malloc(lv_buffer_size, MALLOC_CAP_DEFAULT));
    if (lv_buffer == nullptr)
    {
        ESP_LOGE(TAG, "Failed to allocate lvBuffer on the heap!");
        return;
    }

    m_display = lv_display_create(horizontal_resolution, vertical_resolution);
    lv_display_set_flush_cb(m_display, flush_cb);
    lv_display_set_user_data(m_display, driver);
    lv_display_set_buffers(m_display, lv_buffer, NULL, lv_buffer_size, LV_DISPLAY_RENDER_MODE_FULL);
}

LedMatrix::~LedMatrix()
{
}

lv_display_t *LedMatrix::get_lv_display()
{
    return m_display;
}

void LedMatrix::set_brightness(uint8_t brightness)
{
    Hub75Driver *driver = static_cast<Hub75Driver*>(lv_display_get_user_data(m_display));
    driver->set_brightness(brightness);
}

void LedMatrix::set_intensity(float intensity)
{
    Hub75Driver *driver = static_cast<Hub75Driver*>(lv_display_get_user_data(m_display));
    driver->set_intensity(intensity);
}
