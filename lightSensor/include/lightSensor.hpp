#pragma once

#include "esp_err.h"
#include <driver/i2c_master.h>

#ifdef __cplusplus
extern "C"
{
#endif

namespace macdap
{
    typedef enum {
        ContinuouslyHResolutionMode     = 0x10,   // Command to set measure mode as Continuously H-Resolution mode
        ContinuouslyHResolutionMode2    = 0x11,   // Command to set measure mode as Continuously H-Resolution mode2
        ContinuouslyLResolutionMode     = 0x13,   // Command to set measure mode as Continuously L-Resolution mode
        OneTimeHResolutionMode          = 0x20,   // Command to set measure mode as One Time H-Resolution mode
        OneTimeHResolutionMode2         = 0x21,   // Command to set measure mode as One Time H-Resolution mode2
        OneTimeLResolutionMode          = 0x23,   // Command to set measure mode as One Time L-Resolution mode
    } measure_mode_t;

    class LightSensor
    {

    private:
        i2c_master_dev_handle_t m_i2cDeviceHandle;
        LightSensor();
        ~LightSensor();

    public:
        LightSensor(LightSensor const&) = delete;
        void operator=(LightSensor const &) = delete;
        static LightSensor &getInstance()
        {
            static LightSensor instance;
            return instance;
        }

        esp_err_t reset();
        esp_err_t powerDown();
        esp_err_t powerOn();
        esp_err_t setMeasureMode(const measure_mode_t measureMode);
        esp_err_t getIlluminance(float *illuminance);
        esp_err_t setMeasureTime(const uint8_t measureTime);

    };
}

#ifdef __cplusplus
}
#endif