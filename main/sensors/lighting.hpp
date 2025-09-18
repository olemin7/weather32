#pragma once
#include <esp_event.h>
#include <stdint.h>

namespace lighting
{
    constexpr auto ADC_VALUE_MAX = 4096;
    void init();
    void set_adc_min(int val);
    void set_adc_max(int val);
    int get_adc_min();
    int get_adc_max();
}
