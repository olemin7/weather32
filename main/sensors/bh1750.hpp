#pragma once
#include <memory>
#include "sensor_cb.hpp"

namespace bh1750 {
    class sensor
    {
        private:
            std::unique_ptr<sensor_cb::timed_cb<uint16_t>> handler_;
            i2c_dev_t dev_;
            bool get_value(uint16_t &value);

        public:
            sensor(sensor_cb::on_success_cb_t<uint16_t> &&on_success, sensor_cb::on_error_cb_t &&on_error);
    };
}