
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

// void bme680_test(void *pvParameters)
// {
   

//     TickType_t last_wakeup = xTaskGetTickCount();

//     bme680_values_float_t values;
//     while (1)
//     {
//         // trigger the sensor to start one TPHG measurement cycle
//         if (bme680_force_measurement(&sensor) == ESP_OK)
//         {
//             // passive waiting until measurement results are available
//             vTaskDelay(duration);

//             // get the results and do something with them
//             if (bme680_get_results_float(&sensor, &values) == ESP_OK)
//                 printf("BME680 Sensor: %.2f °C, %.2f %%, %.2f hPa, %.2f Ohm\n",
//                         values.temperature, values.humidity, values.pressure, values.gas_resistance);
//         }
//         // passive waiting until 1 second is over
//         vTaskDelayUntil(&last_wakeup, pdMS_TO_TICKS(1000));
//     }
// }

namespace bme680
{
    void init()
    {
        ESP_ERROR_CHECK(i2cdev_init());
    }

    bme680::bme680(on_success_t &&on_success, on_error_t &&on_error){
        memset(&sensor, 0, sizeof(bme680_t));
        ESP_ERROR_CHECK(bme680_init_desc(&sensor, ADDR, I2C_NUM_0, static_cast<gpio_num_t>(CONFIG_I2C_MASTER_SDA_IO), static_cast<gpio_num_t>(CONFIG_I2C_MASTER_SCL_IO)));

        // init the sensor
        ESP_ERROR_CHECK(bme680_init_sensor(&sensor));

        bme680_set_oversampling_rates(&sensor, BME680_OSR_4X, BME680_OSR_4X, BME680_OSR_2X);

        // Change the IIR filter size for temperature and pressure to 7.
        bme680_set_filter_size(&sensor, BME680_IIR_SIZE_7);

        bme680_use_heater_profile(&sensor, BME680_HEATER_NOT_USED);
        // as long as sensor configuration isn't changed, duration is constant
        uint32_t duration;
        bme680_get_measurement_duration(&sensor, &duration);
        auto duration_ms = std::chrono::milliseconds(pdTICKS_TO_MS(duration));
        ESP_LOGI(TAG, "measurement duration: %d ms", duration_ms.count());
        if (bme680_force_measurement(&sensor) == ESP_OK){

            timer_p = std::make_unique<idf::esp_timer::ESPTimer>([this, on_success, on_error]()
                                                                 { ESP_LOGI(TAG, "triggered");
                bme680_values_float_t values;
                if (bme680_get_results_float(&sensor, &values) == ESP_OK){
                    ESP_LOGI(TAG,"BME680 Sensor: %.2f °C, %.2f %%, %.2f hPa, %.2f Ohm\n",
                             values.temperature, values.humidity, values.pressure, values.gas_resistance);
                    on_success(values.temperature, values.humidity, values.pressure, values.gas_resistance);
                }
                else
                {
                    ESP_LOGE(TAG, "failed to get results");
                    on_error("failed to get results");
                } });
            timer_p->start(duration_ms);
        }else{
            ESP_LOGE(TAG, "failed to trigger measurement");
            on_error("failed to trigger measurement");
        }
    }
    

    bme680::~bme680()
    {
        bme680_free_desc(&sensor);
    }
}
