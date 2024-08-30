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
#include <chrono>
#include "driver/i2c.h"
#include "esp_err.h"
#include "i2c_bus.h"
#include "utils.hpp"
#include "bme280.h"
#include "esp_timer_cxx.hpp"

namespace sensors {

typedef struct {
    float temperature;
    float humidity;
    float pressure;
} bme280_t;

class CBME260_wrapper {
 protected:
    using cb_t                                                = std::function<void(const bme280_t&&)>;
    static constexpr auto                     BME280_RETRY_TM = std::chrono::milliseconds(100);
    bme280_handle_t                           bme280_id;
    std::unique_ptr<idf::esp_timer::ESPTimer> retry_tm_;
    cb_t                                      cb_;

    void read();

 public:
    CBME260_wrapper(i2c_bus_handle_t& i2c_bus);
    ~CBME260_wrapper();

    void set_mode(bme280_sensor_mode mode);
    void init();

    void      begin(cb_t&& cb);
    esp_err_t take_forced_measurement();
};

class CBME260_wrapper_forced: private utils::generic_sensor<bme280_t> {
 private:
    CBME260_wrapper bme_;

 public:
    CBME260_wrapper_forced(i2c_bus_handle_t& i2c_bus, cb_t&& cb);
    ~CBME260_wrapper_forced();
};

} // namespace sensors
