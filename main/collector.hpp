/*
 * sensors.h
 *
 *  Created on: Jun 24, 2023
 *      Author: oleksandr
 */

#pragma once

#include <functional>
#include <memory>
#include <stdint.h>
#include <optional>
#include <chrono>
#include "driver/i2c.h"
#include "i2c_bus.h"
#include <bme280_wrapper.hpp>

namespace sensors {

typedef struct {
    std::optional<bme280_t> bme280;
} result_t;

class CCollector {
 public:
    using cb_t = std::function<void(const result_t&)>;
    CCollector(cb_t&& cb);
    ~CCollector();
    const result_t& get() const;
    bool            ready() const;

 private:
    result_t                                result_;
    cb_t                                    cb_;
    i2c_bus_handle_t                        i2c_bus;
    std::unique_ptr<CBME260_wrapper_forced> bme280_;

    void updated();
};

} // namespace sensors
