#pragma once
#include "sensor_cb.hpp"
#include "esp_timer_cxx.hpp"
#include <functional>
#include <memory>
#include <bme680.h>
#include <string>
/*
BME680_I2C_ADDRESS_x77
*/
namespace bme680
{
    using sensor_value_t = bme680_values_float_t;
    class sensor
    {
    private:
        std::unique_ptr<sensor_cb::timed_cb<sensor_value_t>> handler_;
        bme680_t dev_;
        bool get_value(sensor_value_t &value);

    public:
        sensor(sensor_cb::on_success_cb_t<sensor_value_t> &&on_success, sensor_cb::on_error_cb_t &&on_error);
        ~sensor();
    };
}