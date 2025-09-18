#pragma once
#include <functional>
#include <sys/time.h>
#include <string>
#include "esp_timer_cxx.hpp"

namespace clock_tm
{
    using cb_t = std::function<void(const struct tm &timeinfo)>;
    class clock
    {
    private:
        idf::esp_timer::ESPTimer minute_timer_;

    public:
        clock(cb_t &&cb);
    };

    void update_time_zone(const std::string &tz);
    std::string get_tz();
}
