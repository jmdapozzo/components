#include "statusBuzzer.hpp"
#include <esp_log.h>
#include <esp_err.h>

using namespace macdap;

static const char *TAG = "statusBuzzer";

// Pattern definitions (on_duration_ms, off_duration_ms)
struct BuzzerPattern
{
    uint32_t on_duration_ms;
    uint32_t off_duration_ms;
};

// Chirp = short burst, Beep = longer tone
static const BuzzerPattern patterns[] = {
    [static_cast<int>(BuzzerStatus::Silent)]        = {0,    0},        // Silent
    [static_cast<int>(BuzzerStatus::SlowChirp)]     = {50,   1950},     // Short chirp every 2s
    [static_cast<int>(BuzzerStatus::MediumChirp)]   = {50,   950},      // Short chirp every 1s
    [static_cast<int>(BuzzerStatus::FastChirp)]     = {50,   450},      // Short chirp every 0.5s
    [static_cast<int>(BuzzerStatus::SlowBeep)]      = {200,  1800},     // Beep every 2s
    [static_cast<int>(BuzzerStatus::MediumBeep)]    = {200,  800},      // Beep every 1s
    [static_cast<int>(BuzzerStatus::FastBeep)]      = {200,  300},      // Beep every 0.5s
    [static_cast<int>(BuzzerStatus::Continuous)]    = {0,    0},        // Continuous tone
};

StatusBuzzer::StatusBuzzer()
    : m_ledc_channel(LEDC_CHANNEL_0)
    , m_ledc_timer(LEDC_TIMER_0)
    , m_gpio_num(GPIO_NUM_NC)
    , m_pattern_timer(nullptr)
    , m_current_status(BuzzerStatus::Silent)
    , m_tone_active(false)
{
}

StatusBuzzer::~StatusBuzzer()
{
    stop();
    if (m_pattern_timer != nullptr)
    {
        esp_timer_stop(m_pattern_timer);
        esp_timer_delete(m_pattern_timer);
        m_pattern_timer = nullptr;
    }
}

esp_err_t StatusBuzzer::init(gpio_num_t gpio_num, ledc_timer_t timer, ledc_channel_t channel)
{
    ESP_LOGI(TAG, "Initializing StatusBuzzer on GPIO %d", gpio_num);

    m_gpio_num = gpio_num;
    m_ledc_timer = timer;
    m_ledc_channel = channel;

    // Configure LEDC timer for 4000Hz square wave
    ledc_timer_config_t ledc_timer_cfg = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_DUTY_RESOLUTION,
        .timer_num        = m_ledc_timer,
        .freq_hz          = BUZZER_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    esp_err_t ret = ledc_timer_config(&ledc_timer_cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure LEDC timer: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure LEDC channel
    ledc_channel_config_t ledc_channel_cfg = {
        .gpio_num       = m_gpio_num,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = m_ledc_channel,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = m_ledc_timer,
        .duty           = 0,
        .hpoint         = 0,
        .flags          = {
            .output_invert = 0
        }
    };
    ret = ledc_channel_config(&ledc_channel_cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure LEDC channel: %s", esp_err_to_name(ret));
        return ret;
    }

    // Create the pattern timer
    const esp_timer_create_args_t timer_args = {
        .callback = &StatusBuzzer::pattern_timer_callback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "buzzer_pattern",
        .skip_unhandled_events = false
    };

    ret = esp_timer_create(&timer_args, &m_pattern_timer);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create pattern timer: %s", esp_err_to_name(ret));
        return ret;
    }

    // Start with buzzer off
    tone_off();

    ESP_LOGI(TAG, "StatusBuzzer initialized successfully");
    return ESP_OK;
}

void StatusBuzzer::tone_on()
{
    // Set 50% duty cycle for square wave
    uint32_t duty = ((1 << LEDC_DUTY_RESOLUTION) - 1) * SQUARE_WAVE_DUTY_CYCLE / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, m_ledc_channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, m_ledc_channel);
    m_tone_active = true;
}

void StatusBuzzer::tone_off()
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, m_ledc_channel, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, m_ledc_channel);
    m_tone_active = false;
}

void StatusBuzzer::set_status(BuzzerStatus status)
{
    if (status >= BuzzerStatus::Count)
    {
        ESP_LOGE(TAG, "Invalid buzzer status");
        return;
    }

    ESP_LOGI(TAG, "Setting status to %d", static_cast<int>(status));

    // Stop current pattern
    if (m_pattern_timer != nullptr)
    {
        esp_timer_stop(m_pattern_timer);
    }
    tone_off();

    m_current_status = status;
    start_pattern(status);
}

void StatusBuzzer::stop()
{
    if (m_pattern_timer != nullptr)
    {
        esp_timer_stop(m_pattern_timer);
    }
    tone_off();
    m_current_status = BuzzerStatus::Silent;
}

void StatusBuzzer::start_pattern(BuzzerStatus status)
{
    if (status == BuzzerStatus::Silent)
    {
        tone_off();
        return;
    }

    if (status == BuzzerStatus::Continuous)
    {
        tone_on();
        return;
    }

    // Start pattern with tone on
    const BuzzerPattern &pattern = patterns[static_cast<int>(status)];
    if (pattern.on_duration_ms > 0)
    {
        tone_on();
        // Schedule the first transition
        esp_timer_start_once(m_pattern_timer, pattern.on_duration_ms * 1000);
    }
}

void StatusBuzzer::pattern_timer_callback(void *arg)
{
    StatusBuzzer *buzzer = static_cast<StatusBuzzer*>(arg);
    buzzer->handle_pattern_timer();
}

void StatusBuzzer::handle_pattern_timer()
{
    const BuzzerPattern &pattern = patterns[static_cast<int>(m_current_status)];

    if (m_tone_active)
    {
        // Currently on, switch to off
        tone_off();
        if (pattern.off_duration_ms > 0)
        {
            esp_timer_start_once(m_pattern_timer, pattern.off_duration_ms * 1000);
        }
    }
    else
    {
        // Currently off, switch to on
        tone_on();
        if (pattern.on_duration_ms > 0)
        {
            esp_timer_start_once(m_pattern_timer, pattern.on_duration_ms * 1000);
        }
    }
}
