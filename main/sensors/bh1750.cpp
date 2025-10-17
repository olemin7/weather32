#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <bh1750.h>
#include "bh1750.hpp"

#include <string.h>

#if defined(CONFIG_BH1750_I2C_ADDRESS_LO)
#define ADDR BH1750_ADDR_LO
#endif
#if defined(CONFIG_BH1750_I2C_ADDRESS_HI)
#define ADDR BH1750_ADDR_HI
#endif

static const char *TAG = "bh1750";
using namespace std::chrono_literals;
constexpr auto read_timeout = 200ms;
constexpr auto retry_timeout = 10ms;
constexpr auto retry_count = 3;
namespace bh1750
{
    sensor::sensor(sensor_cb::on_success_cb_t<uint16_t> &&on_success, sensor_cb::on_error_cb_t &&on_error)
    {
        ESP_LOGI(TAG, "init");

        memset(&dev_, 0, sizeof(i2c_dev_t)); // Zero descriptor

        if ((bh1750_init_desc(&dev_, ADDR, I2C_NUM_0, static_cast<gpio_num_t>(CONFIG_I2CDEV_DEFAULT_SDA_PIN), static_cast<gpio_num_t>(CONFIG_I2CDEV_DEFAULT_SCL_PIN)) != ESP_OK) ||
            (bh1750_setup(&dev_, BH1750_MODE_CONTINUOUS, BH1750_RES_HIGH) != ESP_OK))
        {
            ESP_LOGE(TAG, "sensor initialization failed");
            on_error();
            return;
        }

        handler_ = std::make_unique<sensor_cb::timed_cb<uint16_t>>(TAG, std::bind(&sensor::get_value, this, std::placeholders::_1),
                                                                   std::move(on_success), std::move(on_error), read_timeout, retry_count,retry_timeout);
    }

    bool sensor::get_value(uint16_t &value)
    {
        if (bh1750_read(&dev_, &value) != ESP_OK)
        {
            ESP_LOGE(TAG, "Could not read lux data");
            return false;
        }
        else
        {
            ESP_LOGI(TAG, "Lux: %d", value);
            return true;
        }
    }
    sensor::~sensor()
    {
        bh1750_power_down(&dev_);
        bh1750_free_desc(&dev_);
        ESP_LOGI(TAG, "deinit");
    }
}
