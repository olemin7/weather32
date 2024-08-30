/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "collector.hpp"
#include "lwip/sys.h"
#include "sdkconfig.h"

#include <memory>
#include <stdio.h>
#include <utility>
#include "esp_log.h"

namespace sensors {

#define I2C_MASTER_NUM I2C_NUM_0 /*!< I2C port number for master bme280 */
#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master do not need buffer */
#define I2C_MASTER_FREQ_HZ 100000 /*!< I2C master clock frequency */
static const char* TAG = "SENSORS";

CCollector::CCollector(cb_t&& cb)
    : cb_(std::move(cb)) {
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
        bme280_ = std::make_unique<CBME260_wrapper_forced>(i2c_bus, [this](auto res) {
            result_.bme280 = res;
            updated();
        });
    }
}
CCollector::~CCollector() {
    ESP_LOGI(TAG, "CManager::Impl deleted");
    bme280_.reset();
    i2c_bus_delete(&i2c_bus);
}

const result_t& CCollector::get() const {
    return result_;
}

bool CCollector::ready() const {
    if (CONFIG_PRESENT_BME280 && !result_.bme280) {
        return false;
    }
    return true;
}

void CCollector::updated() {
    if (ready()) {
        ESP_LOGI(TAG, "CManager call cb_");
        cb_(result_);
    }
}

} // namespace sensors
