#pragma once
#include <memory>
#include "sensor_cb.hpp"

namespace bh1750 {
    using sensor_value_t = uint16_t;
    class sensor
    {
        private:
            std::unique_ptr<sensor_cb::timed_cb<sensor_value_t>> handler_;
            i2c_dev_t dev_;
            bool get_value(sensor_value_t &value);

        public:
            sensor(sensor_cb::on_success_cb_t<sensor_value_t> &&on_success, sensor_cb::on_error_cb_t &&on_error);
            ~sensor();
    };
}