/*
 * blink.cpp
 *
 *  Created on: Jul 1, 2024
 *      Author: oleksandr
 */
#include "htu2x.hpp"
#include <inttypes.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <si7021.h>
#include <string.h>
#include <chrono>
#include "esp_log.h"
#include "sensor_event.hpp"
#include "utils/average_treshold.hpp"
#include "utils/utils.hpp"

using namespace std::chrono_literals;
static const char *TAG = "si7021";

constexpr auto TEMPERATURE_THRESHOLD = 0.1;
constexpr auto HUMIDITY_THRESHOLD = 0.5;

static void
task(void *pvParameters)
{
    i2c_dev_t dev;
    memset(&dev, 0, sizeof(i2c_dev_t));

    ESP_ERROR_CHECK(si7021_init_desc(&dev, I2C_NUM_0, static_cast<gpio_num_t>(CONFIG_I2C_MASTER_SDA_IO), static_cast<gpio_num_t>(CONFIG_I2C_MASTER_SCL_IO)));

    gpio_dump_io_configuration(stdout, (1ULL << 8) | (1ULL << 9) | (1ULL << CONFIG_I2C_MASTER_SDA_IO) | (1ULL << CONFIG_I2C_MASTER_SCL_IO));
    uint64_t serial;
    si7021_device_id_t id;

    ESP_ERROR_CHECK(si7021_get_serial(&dev, &serial, false));
    ESP_ERROR_CHECK(si7021_get_device_id(&dev, &id));

    printf("Device: ");
    switch (id)
    {
    case SI_MODEL_SI7013:
        ESP_LOGD(TAG, "Si7013");
        break;
    case SI_MODEL_SI7020:
        ESP_LOGD(TAG, "Si7020");
        break;
    case SI_MODEL_SI7021:
        ESP_LOGD(TAG, "Si7021");
        break;
    case SI_MODEL_SAMPLE:
        ESP_LOGD(TAG, "Engineering sample");
        break;
    default:
        ESP_LOGD(TAG, "Unknown");
    }
    ESP_LOGD(TAG, "Serial number: 0x%08" PRIx32 "%08" PRIx32, (uint32_t)(serial >> 32), (uint32_t)serial);
    bool heater;
    ESP_ERROR_CHECK(si7021_get_heater(&dev, &heater));
    ESP_LOGI(TAG, "si7021_get_heater %d", heater);

    utils::average_treshold_timeout<float, float> temperature_filter(TEMPERATURE_THRESHOLD, 3, std::chrono::seconds(CONFIG_SENSORS_KA_PERIOD_S));
    utils::average_treshold_timeout<float, float> humidity_filter(HUMIDITY_THRESHOLD, 3, std::chrono::seconds(CONFIG_SENSORS_KA_PERIOD_S));

    while (1)
    {
        esp_err_t res;
        float temperature;
        res = si7021_measure_temperature(&dev, &temperature);
        if (res != ESP_OK)
        {
            ESP_LOGE(TAG, "Could not measure temperature: %d (%s)", res, esp_err_to_name(res));
        }
        else
        {
            ESP_LOGD(TAG, "Temperature: %.2f", temperature);
            if (temperature_filter.push(temperature))
            {
                sensor_event::temperature_t event;
                event.val = utils::trimm(temperature_filter.get_average(), 1);
                ESP_ERROR_CHECK(esp_event_post(sensor_event::event, sensor_event::internall_temperature, &event, sizeof(event), portMAX_DELAY));
            }
        }

        float humidity;
        res = si7021_measure_humidity(&dev, &humidity);
        if (res != ESP_OK)
        {
            ESP_LOGE(TAG, "Could not measure humidity: %d (%s)", res, esp_err_to_name(res));
        }
        else
        {
            ESP_LOGD(TAG, "Humidity: %.2f", humidity);
            if (humidity_filter.push(humidity))
            {
                sensor_event::humidity_t event;
                event.val = utils::trimm(humidity_filter.get_average(), 1);
                ESP_ERROR_CHECK(esp_event_post(sensor_event::event, sensor_event::internall_humidity, &event, sizeof(event), portMAX_DELAY));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void echo_temperature(void * /*arg*/, esp_event_base_t /*event_base*/, int32_t /*event_id*/, void *event_data)
{
    const auto temperature = (sensor_event::temperature_t *)event_data;
    ESP_LOGI(TAG, "temperature=%f", temperature->val);
}

void echo_humidity(void * /*arg*/, esp_event_base_t /*event_base*/, int32_t /*event_id*/, void *event_data)
{
    const auto humidity = (sensor_event::humidity_t *)event_data;
    ESP_LOGI(TAG, "humidity=%f", humidity->val);
}

void htu2x::init()
{
    ESP_ERROR_CHECK(i2cdev_init());

    // ESP_ERROR_CHECK(esp_event_handler_register(sensor_event::event, sensor_event::internall_temperature, &echo_temperature, NULL));
    // ESP_ERROR_CHECK(esp_event_handler_register(sensor_event::event, sensor_event::internall_humidity, &echo_humidity, NULL));

    xTaskCreate(&task, TAG, configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL);
}
