/*
 * blink.cpp
 *
 *  Created on: Jul 1, 2024
 *      Author: oleksandr
 */
#include "blink.hpp"
#include <esp_log.h>
#include "sdkconfig.h"
#include "driver/gpio.h"
#include <chrono>
#include <memory>
#include "esp_timer_cxx.hpp"

namespace blink {
using namespace std::literals::chrono_literals;
constexpr auto     BLINK_GPIO = GPIO_NUM_8;
static const char* TAG        = "blink";
class cBlink {
 public:
    cBlink(std::chrono::milliseconds t_on, std::chrono::milliseconds t_off)
        : t_on_(t_on)
        , t_off_(t_off) {
        set_on();
    }

 private:
    std::chrono::milliseconds                 t_on_;
    std::chrono::milliseconds                 t_off_;
    std::unique_ptr<idf::esp_timer::ESPTimer> timer_;
    void                                      set_on();
    void                                      set_off();
};

void cBlink::set_on() {
    ESP_LOGD(TAG, "set_on");
    gpio_set_level(BLINK_GPIO, 0);
    timer_ = std::make_unique<idf::esp_timer::ESPTimer>([this]() { this->set_off(); });
    timer_->start(t_on_);
}

void cBlink::set_off() {
    ESP_LOGD(TAG, "set_off");
    gpio_set_level(BLINK_GPIO, 1);
    timer_ = std::make_unique<idf::esp_timer::ESPTimer>([this]() { this->set_on(); });
    timer_->start(t_off_);
}

void init() {
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

void set(led_state_e state) {
    static std::unique_ptr<cBlink> blink;
    blink.reset();
    ESP_LOGI(TAG, "set state=%d", static_cast<unsigned>(state));
    switch (state) {
        case led_state_e::OFF:
            gpio_set_level(BLINK_GPIO, 1);
            break;
        case led_state_e::ON:
            gpio_set_level(BLINK_GPIO, 0);
            break;
        case led_state_e::SLOW:
            blink = std::make_unique<cBlink>(500ms, 1s);
            break;
        case led_state_e::FAST:
            blink = std::make_unique<cBlink>(100ms, 200ms);
            break;
    }
    //  blink_ptr = std::make_unique<cBlink>(el, pin);
}

} // namespace blink