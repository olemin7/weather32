#pragma once
#include <functional>
#include <chrono>
#include "esp_timer_cxx.hpp"
#include "esp_log.h"
namespace sensor_cb{
    using on_error_cb_t= std::function<void()>;
    template <typename T>
    using on_success_cb_t = std::function<void(T &&value)>;
    template <typename T>
    using getter_t = std::function<bool(T &value)>;

    template <typename T>
    class timed_cb
    {
    protected:
        int retry_;
        std::unique_ptr<idf::esp_timer::ESPTimer> timer_p_;

    public:
        timed_cb(const char *tag,
                 getter_t<T> &&getter,
                 on_success_cb_t<T> &&on_success,
                 on_error_cb_t &&on_error,
                 std::chrono::milliseconds delay,
                 int retry = 0,
                 std::chrono::milliseconds retry_timeout = {})
            : retry_{retry}
        {
            timer_p_ = std::make_unique<idf::esp_timer::ESPTimer>([this, tag, getter, on_success, on_error, retry_timeout]()
                                                                  {
                T value;
                if (getter(value))
                {
                    on_success(std::move(value));
                }
                else
                {
                    ESP_LOGI(tag, "error, retryes remains %d", retry_);
                    if(retry_--){
                        timer_p_->start(retry_timeout);
                    }else{
                        on_error();
                    }
                } }, tag);
            timer_p_->start(delay);
        }
    };
}
