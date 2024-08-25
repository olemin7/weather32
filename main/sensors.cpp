/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sensors.hpp"
#include "sdkconfig.h"

#include <memory>
#include <stdio.h>
#include <utility>
#include "unity.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "bme280.h"
#include "esp_timer_cxx.hpp"

using namespace std::chrono_literals;

namespace sensors {

#define I2C_MASTER_NUM I2C_NUM_0 /*!< I2C port number for master bme280 */
#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master do not need buffer */
#define I2C_MASTER_FREQ_HZ 100000 /*!< I2C master clock frequency */
static const char* TAG = "Sensors";

class CManager::Impl {
 private:
    static constexpr auto                     BME280_RETRY_TM = 100ms;
    i2c_bus_handle_t                          i2c_bus         = NULL;
    bme280_handle_t                           bme280          = NULL;
    cb_t                                      cb_;
    result_t                                  result_;
    std::unique_ptr<idf::esp_timer::ESPTimer> timeout_tm_;
    idf::esp_timer::ESPTimer                  bme280_tm_;

    void reset_data() {
        result_.bme280.reset();
    }

    void trigger_cb(const status_t status) {
        if (cb_) {
            result_.status = status;
            cb_(result_);
            cb_ = nullptr;
        }
        reset_data();
    }

    void updated() {
        if (!result_.bme280) {
            return;
        }
        timeout_tm_.reset();
        trigger_cb(status_t::ok);
    }

    void bme280_handler() {
        bme280_t data;
        if (ESP_OK == bme280_read_temperature(bme280, &data.temperature)) {
            ESP_LOGD(TAG, "temperature:%f ", data.temperature);
            if (ESP_OK == bme280_read_humidity(bme280, &data.humidity)) {
                ESP_LOGD(TAG, "humidity:%f ", data.humidity);
                if (ESP_OK == bme280_read_pressure(bme280, &data.pressure)) {
                    ESP_LOGD(TAG, "pressure:%f\n", data.pressure);
                    result_.bme280 = data;
                    updated();
                    return;
                }
            }
        }
        ESP_LOGI(TAG, "bme280_retry");
        bme280_tm_.start(BME280_RETRY_TM);
    }

 public:
    Impl()
        : bme280_tm_([this]() { bme280_handler(); }) {
        ESP_LOGI(TAG, "CManager::Impl created");
        ESP_LOGD(TAG, "i2c_master_scl_io:%d i2c_master_sda_io:%d", CONFIG_I2C_MASTER_SCL_IO, CONFIG_I2C_MASTER_SDA_IO);
        i2c_config_t conf = {
            .mode          = I2C_MODE_MASTER,
            .sda_io_num    = CONFIG_I2C_MASTER_SDA_IO,
            .scl_io_num    = CONFIG_I2C_MASTER_SCL_IO,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master        = { .clk_speed = I2C_MASTER_FREQ_HZ },
            .clk_flags     = {},
        };
        i2c_bus = i2c_bus_create(I2C_MASTER_NUM, &conf);
        bme280  = bme280_create(i2c_bus, BME280_I2C_ADDRESS_DEFAULT);
    }

    ~Impl() {
        ESP_LOGI(TAG, "CManager::Impl deleted");
        bme280_delete(&bme280);
        i2c_bus_delete(&i2c_bus);
    }

    void init() {
        reset_data();
        ESP_LOGI(TAG, "bme280_default_init:%d", bme280_default_init(bme280));
    }

    void begin(cb_t&& cb, std::chrono::milliseconds timeout) {
        ESP_LOGI(TAG, "begin, tm:%lld", timeout.count());
        reset_data();
        cb_         = std::move(cb);
        timeout_tm_ = std::make_unique<idf::esp_timer::ESPTimer>([this]() {
            ESP_LOGI(TAG, "timeout");
            trigger_cb(status_t::timeout);
        });
        timeout_tm_->start(timeout);
        bme280_tm_.start(0s);
    }

    void stop() {
        timeout_tm_.reset();
        trigger_cb(status_t::stoped);
    }
};

CManager::CManager()  = default;
CManager::~CManager() = default;

void CManager::init() {
    impl_ = std::make_unique<CManager::Impl>();
    impl_->init();
}

void CManager::begin(cb_t&& cb, std::chrono::milliseconds timeout) {
    if (!impl_) {
        init();
    }
    impl_->begin(std::move(cb), timeout);
}
void CManager::stop() {
    impl_->stop();
}
} // namespace sensors
