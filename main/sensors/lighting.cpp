/*
 * blink.cpp
 *
 *  Created on: Jul 1, 2024
 *      Author: oleksandr
 */
#include "lighting.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include "sensor_event.hpp"
#include "utils/average_treshold.hpp"
#include "utils/kvs.hpp"
#include "utils/utils.hpp"

namespace lighting
{
    using namespace sensor_event;
    using namespace std::chrono_literals;

    constexpr auto TAG = "LIGHTING";
    constexpr auto kvs_adc_max = "adc_max";
    constexpr auto kvs_adc_min = "adc_min";

    void task(void *pvParameter)
    {
        ESP_LOGI(TAG, "init");

        adc_oneshot_unit_handle_t adc1_handle;
        //-------------ADC1 Init---------------//

        adc_oneshot_unit_init_cfg_t init_config1 = {
            .unit_id = ADC_UNIT_1,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

        //-------------ADC1 Config---------------//
        adc_oneshot_chan_cfg_t config = {
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config));
        utils::average_treshold_timeout<uint16_t, uint32_t> lighting_filter(CONFIG_LIGHTING_THRESHOLD, 3, std::chrono::seconds(CONFIG_SENSORS_KA_PERIOD_S));
        const auto adc_max = get_adc_max();
        const auto adc_min = get_adc_min();
        ESP_LOGI(TAG, "ADC adc_min=%d, adc_max=%d", adc_min, adc_max);

        while (1)
        {
            int raw;
            ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &raw));
            if (raw) // to mitigate issue WiFi affects ADC1 raw data (IDFGH-6623) espressif/esp-idf#8266
            {
                raw = ADC_VALUE_MAX - raw; // iverted connection
                const auto lux = utils::transform_range(adc_min, adc_max, LUX_MIN, LUX_MAX, raw);

                ESP_LOGD(TAG, "ADC raw=%d,lux=%d", raw, lux);
                if (lighting_filter.push(lux))
                {
                    lighting_t event;
                    event.raw = raw;
                    event.val = lighting_filter.get_average();
                    ESP_LOGD(TAG, "ADC lux: %d", event.val);
                    ESP_ERROR_CHECK(esp_event_post(sensor_event::event, sensor_event::lighting, &event, sizeof(event), portMAX_DELAY));
                }
                vTaskDelay(pdMS_TO_TICKS(CONFIG_LIGHTING_REFRESH));
            }
            else
            {
                vTaskDelay(pdMS_TO_TICKS(CONFIG_LIGHTING_REFRESH / 2));
            }
        }
    }

#if 0
   void echo(void * /*arg*/, esp_event_base_t /*event_base*/, int32_t /*event_id*/, void *event_data)
    {
        const auto update = (sensor_event::lighting_t *)event_data;

        ESP_LOGI(TAG, "adc=%d, lux=%u", update->raw, update->lux);
    }
.... 
ESP_ERROR_CHECK(esp_event_handler_register(sensor_event::event, sensor_event::lighting, &echo, NULL));

#endif

    void init()
    {
        ESP_LOGI(TAG, "starting");
        //    esp_log_level_set(TAG, ESP_LOG_DEBUG);
        xTaskCreate(&task, TAG, configMINIMAL_STACK_SIZE + 1024, NULL, 5, NULL);
    }

    void set_adc_min(int val)
    {
        ESP_LOGI(TAG, "min %d", val);
        auto kvss = kvs::handler(TAG);
        if (ESP_OK != kvss.set_value(kvs_adc_min, val))
        {
            ESP_LOGE(TAG, "error wr");
        }
    }

    void set_adc_max(int val)
    {
        ESP_LOGI(TAG, "max %d", val);
        auto kvss = kvs::handler(TAG);
        if (ESP_OK != kvss.set_value(kvs_adc_max, val))
        {
            ESP_LOGE(TAG, "error wr");
        }
    }

    int get_adc_max()
    {
        auto kvss = kvs::handler(TAG);
        int val;
        kvss.get_value_or(kvs_adc_max, val, CONFIG_LIGHTING_MAX_RAW);
        return val;
    }

    int get_adc_min()
    {
        auto kvss = kvs::handler(TAG);
        int val;
        kvss.get_value_or(kvs_adc_min, val, CONFIG_LIGHTING_MIN_RAW);
        return val;
    }
}
