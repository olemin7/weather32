/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bme280_wrapper.hpp"
#include "lwip/sys.h"
#include "sdkconfig.h"

#include <memory>
#include <stdio.h>
#include <utility>
#include "esp_log.h"

using namespace std::chrono_literals;

namespace sensors {

static const char* TAG = "BME280";

void CBME260_wrapper::read() {
    bme280_t data;
    if (ESP_OK == bme280_read_temperature(bme280_id, &data.temperature)) {
        ESP_LOGD(TAG, "temperature:%f ", data.temperature);
        if (ESP_OK == bme280_read_humidity(bme280_id, &data.humidity)) {
            ESP_LOGD(TAG, "humidity:%f ", data.humidity);
            if (ESP_OK == bme280_read_pressure(bme280_id, &data.pressure)) {
                ESP_LOGD(TAG, "pressure:%f\n", data.pressure);
                retry_tm_.reset();
                cb_(std::move(data));
                return;
            }
        }
    }
    ESP_LOGI(TAG, "bme280_retry");
    if (!retry_tm_) {
        retry_tm_ = std::make_unique<idf::esp_timer::ESPTimer>([this]() { read(); });
    }
    retry_tm_->start(BME280_RETRY_TM);
}

CBME260_wrapper::CBME260_wrapper(i2c_bus_handle_t& i2c_bus) {
    bme280_id = bme280_create(i2c_bus, BME280_I2C_ADDRESS_DEFAULT);
}
CBME260_wrapper::~CBME260_wrapper() {
    bme280_delete(&bme280_id);
}

void CBME260_wrapper::set_mode(bme280_sensor_mode mode) {
    const auto res = bme280_set_sampling(bme280_id, mode, BME280_SAMPLING_X16, BME280_SAMPLING_X16, BME280_SAMPLING_X16,
        BME280_FILTER_OFF, BME280_STANDBY_MS_0_5);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "bme280_set_mode %d, result=%d", static_cast<int>(mode), static_cast<int>(res));
    }
}
void CBME260_wrapper::init() {
    const auto res = bme280_default_init(bme280_id);
    ESP_LOGI(TAG, "bme280_default_init:%d", res);
}

void CBME260_wrapper::begin(cb_t&& cb) {
    cb_ = std::move(cb);
    read();
}
esp_err_t CBME260_wrapper::take_forced_measurement() {
    return bme280_take_forced_measurement(bme280_id);
}

CBME260_wrapper_forced::CBME260_wrapper_forced(i2c_bus_handle_t& i2c_bus, cb_t&& cb)
    : generic_sensor<bme280_t>(std::move(cb))
    , bme_(i2c_bus) {
    bme_.init();
    bme_.set_mode(BME280_MODE_FORCED);
    bme_.begin([this](auto res) { set(res); });

    ESP_LOGI(TAG, "bme280_take_forced_measurement:%d", bme_.take_forced_measurement());
}

CBME260_wrapper_forced::~CBME260_wrapper_forced() {
    bme_.set_mode(BME280_MODE_SLEEP);
}

} // namespace sensors
