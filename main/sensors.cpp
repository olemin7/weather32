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
static const char* TAG = "SENSORS";

class CBME260_wrapper {
 protected:
    using cb_t                                                = std::function<void(const bme280_t&&)>;
    static constexpr auto                     BME280_RETRY_TM = 100ms;
    bme280_handle_t                           bme280_id;
    std::unique_ptr<idf::esp_timer::ESPTimer> retry_tm_;
    cb_t                                      cb_;

    void read() {
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

 public:
    CBME260_wrapper(i2c_bus_handle_t& i2c_bus) {
        bme280_id = bme280_create(i2c_bus, BME280_I2C_ADDRESS_DEFAULT);
    }
    ~CBME260_wrapper() {
        bme280_delete(&bme280_id);
    }

    void set_mode(bme280_sensor_mode mode) {
        const auto res = bme280_set_sampling(bme280_id, mode, BME280_SAMPLING_X16, BME280_SAMPLING_X16,
            BME280_SAMPLING_X16, BME280_FILTER_OFF, BME280_STANDBY_MS_0_5);
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "bme280_set_mode %d, result=%d", static_cast<int>(mode), static_cast<int>(res));
        }
    }
    void init() {
        const auto res = bme280_default_init(bme280_id);
        ESP_LOGI(TAG, "bme280_default_init:%d", res);
    }

    void begin(cb_t&& cb) {
        cb_ = std::move(cb);
        read();
    }
};

class CBME260_wrapper_forced: private CBME260_wrapper {
 public:
    CBME260_wrapper_forced(i2c_bus_handle_t& i2c_bus)
        : CBME260_wrapper(i2c_bus) {
        init();
        set_mode(BME280_MODE_FORCED);
    }
    ~CBME260_wrapper_forced() {
        set_mode(BME280_MODE_SLEEP);
    }
    void begin(cb_t&& cb) {
        const auto res = bme280_take_forced_measurement(bme280_id);
        ESP_LOGI(TAG, "bme280_take_forced_measurement:%d", res);
        CBME260_wrapper::begin(std::move(cb));
    }
};

class CManager::Impl {
 private:
    i2c_bus_handle_t                          i2c_bus;
    cb_t                                      cb_;
    result_t                                  result_;
    std::unique_ptr<idf::esp_timer::ESPTimer> timeout_tm_;
    std::unique_ptr<CBME260_wrapper_forced>   bme280_;

    void reset_data() {
        result_.bme280.reset();
    }

    void trigger_cb(const status_t status) {
        timeout_tm_.reset();
        if (cb_) {
            result_.status = status;
            cb_(result_);
            cb_ = nullptr;
        }
        reset_data();
    }

    void updated() {
        if (CONFIG_PRESENT_BME280 && !result_.bme280) {
            return;
        }
        trigger_cb(status_t::ok);
    }

 public:
    Impl() {
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
        if (CONFIG_PRESENT_BME280) {
            bme280_ = std::make_unique<CBME260_wrapper_forced>(i2c_bus);
        }
    }

    ~Impl() {
        ESP_LOGI(TAG, "CManager::Impl deleted");
        i2c_bus_delete(&i2c_bus);
    }

    void init() {
        reset_data();
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
        if (bme280_) {
            bme280_->begin([this](auto res) {
                result_.bme280 = std::move(res);
                updated();
            });
        }
        updated(); // for dummy case
    }

    void stop() {
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
