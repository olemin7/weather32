#include "adc.hpp"
#include "esp_log.h"
#include "driver/adc.h"
#include "sdkconfig.h"

static const char *TAG = "adc";

namespace adc
{
    using namespace std::chrono_literals;

    sensor::sensor(sensor_cb::on_success_cb_t<sensor_value_t> &&on_success,
                   sensor_cb::on_error_cb_t &&on_error)
        : count_(CONFIG_ADC_READ_COUNT), average_(CONFIG_ADC_READ_COUNT)
    {
        ESP_LOGE(TAG, "init");
        adc_oneshot_unit_init_cfg_t init_config1 = {
            .unit_id = ADC_UNIT_1,
        };
        if (adc_oneshot_new_unit(&init_config1, &adc_handle_) != ESP_OK) {
            on_error();
            return;
        }

        adc_oneshot_chan_cfg_t config = {
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };

        if (adc_oneshot_config_channel(adc_handle_, ADC_CHANNEL_0, &config) != ESP_OK) {
            on_error();
            return;
        }

        timer_p_ = std::make_unique<idf::esp_timer::ESPTimer>([this, on_success, on_error]() {
          
            get_value();
                if(count_--){
                    timer_p_->start(std::chrono::milliseconds(CONFIG_ADC_READ_TIMEOUT));
                }
                else
                {
                    if(average_.get_size() == 0){
                        on_error();
                    }else{
                        auto average = average_.get_average();
                        ESP_LOGI(TAG, "ADC count: %d, average: %d", average_.get_size(), average);
                        on_success(std::move(average));
                    }
                }
        }, TAG);
        
        timer_p_->start(0s);
    }
    
    sensor::~sensor()
    {
        if (adc_handle_)
        {
            adc_oneshot_del_unit(adc_handle_);
        }
        ESP_LOGE(TAG, "deinit");
    }
    
    bool sensor::get_value()
    {
        int adc_raw;
        esp_err_t ret = adc_oneshot_read(adc_handle_, ADC_CHANNEL_0, &adc_raw);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ADC read failed: %s", esp_err_to_name(ret));
            return false;
        }
        
        average_.push(adc_raw);
        return true;
    }
}