#pragma once
#include "esp_timer_cxx.hpp"
#include <functional>
#include <memory>
#include <bme680.h>
#include <string>
/*
BME680_I2C_ADDRESS_x77
*/
namespace bme680
{
    typedef std::function<void(std::string error)> on_error_t;
    typedef std::function<void(float temperature, float pressure, float humidity, float gas_resistance)> on_success_t;

    class bme680
    {
    
    private:
        std::unique_ptr<idf::esp_timer::ESPTimer> timer_p;
        bme680_t sensor;

    public:
        bme680(on_success_t &&,on_error_t &&);
        ~bme680();
    };
    

    
    void init();
}