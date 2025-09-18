#pragma once

#include <functional>
#include <chrono>
#include "esp_timer_cxx.hpp"

namespace utils
{

    template <typename T>
    class puller
    {
    private:
        using getter_t = std::function<bool(T &val)>;
        using onchange_cb_t = std::function<void(const T &val)>;
        bool first_force_ = true;
        T val_;
        getter_t getter_;
        onchange_cb_t onchange_cb_;
        idf::esp_timer::ESPTimer minute_timer_;

    public:
        puller(getter_t &&getter, const std::chrono::milliseconds period, onchange_cb_t &&onchange_cb)
            : getter_(getter), onchange_cb_(onchange_cb), minute_timer_([this]()
                                                                        { 
                                                                            T tmp;
                                                                            if(getter_(tmp)&&(first_force_ ||(tmp!=val_))){
                                                                                first_force_=false;
                                                                                val_=tmp;
                                                                                onchange_cb_(val_);
                                                                            } }, "puller")
        {
            minute_timer_.start_periodic(period);
        }
        ~puller() = default;
    };

}
