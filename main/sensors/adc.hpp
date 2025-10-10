#pragma once
#include <memory>
#include <chrono>
#include "esp_timer_cxx.hpp"
#include "sensor_cb.hpp"
#include "utils/average_treshold.hpp"
#include "esp_adc/adc_oneshot.h"

namespace adc
{
    using sensor_value_t = int;
    class sensor
    {
    private:
        int count_;
        std::unique_ptr<idf::esp_timer::ESPTimer> timer_p_;
        utils::average<sensor_value_t> average_;
        adc_oneshot_unit_handle_t adc_handle_;

        bool get_value();

    public:
        sensor(sensor_cb::on_success_cb_t<sensor_value_t> &&on_success, sensor_cb::on_error_cb_t &&on_error);
        ~sensor();
    };
}