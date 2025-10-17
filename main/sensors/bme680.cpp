
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include "sdkconfig.h"
#include "bme680.hpp"
#include "esp_log.h"

#if defined(CONFIG_BME680_I2C_ADDRESS_x77)
#define ADDR BME680_I2C_ADDR_1
#else
#define ADDR BME680_I2C_ADDR_0
#endif

static const char *TAG = "bme680";

using namespace std::chrono_literals;
constexpr auto retry_timeout = 10ms;
constexpr auto retry_count = 3;

namespace bme680
{

    sensor::sensor(sensor_cb::on_success_cb_t<sensor_value_t> &&on_success, sensor_cb::on_error_cb_t &&on_error)
    {
        ESP_LOGI(TAG, "init");
        memset(&dev_, 0, sizeof(bme680_t));

        if (bme680_init_desc(&dev_, ADDR, I2C_NUM_0, static_cast<gpio_num_t>(CONFIG_I2CDEV_DEFAULT_SDA_PIN), static_cast<gpio_num_t>(CONFIG_I2CDEV_DEFAULT_SCL_PIN)) != ESP_OK ||
            bme680_init_sensor(&dev_) != ESP_OK ||
            bme680_set_oversampling_rates(&dev_, BME680_OSR_4X, BME680_OSR_4X, BME680_OSR_2X) != ESP_OK ||
            bme680_set_filter_size(&dev_, BME680_IIR_SIZE_0) != ESP_OK ||
            bme680_use_heater_profile(&dev_, BME680_HEATER_NOT_USED) != ESP_OK)
        {
            ESP_LOGE(TAG, "sensor initialization failed");
            on_error();
            return;
        }

        uint32_t duration;
        if (bme680_get_measurement_duration(&dev_, &duration) != ESP_OK ||
            bme680_force_measurement(&dev_) != ESP_OK)
        {
            ESP_LOGE(TAG, "failed to start measurement");
            on_error();
            return;
        }

        const auto read_timeout = std::chrono::milliseconds(pdTICKS_TO_MS(duration)) + 10ms;
        ESP_LOGI(TAG, "measurement duration: %d ms", read_timeout.count());
        handler_ = std::make_unique<sensor_cb::timed_cb<sensor_value_t>>(TAG, std::bind(&sensor::get_value, this, std::placeholders::_1),
                                                                         std::move(on_success), std::move(on_error), read_timeout, retry_count, retry_timeout);
    }

    bool sensor::get_value(sensor_value_t &value)
    {
        if (bme680_get_results_float(&dev_, &value) == ESP_OK)
        {
            ESP_LOGI(TAG, "BME680 Sensor: %.2f °C, %.2f %%, %.2f hPa, %.2f Ohm",
                     value.temperature, value.humidity, value.pressure, value.gas_resistance);

            if (value.temperature > 100 || value.temperature < -100 || value.humidity > 200 || value.humidity < 0 || value.pressure > 2000 || value.pressure < 0)
            {
                ESP_LOGE(TAG, "value is out expected range");
            }

            return true;
        }
        else
        {
            ESP_LOGE(TAG, "failed to get results");
            return false;
        }
    }

    sensor::~sensor()
    {
        bme680_use_heater_profile(&dev_, BME680_HEATER_NOT_USED);
        bme680_free_desc(&dev_);
        ESP_LOGI(TAG, "deinit");
    }
}
