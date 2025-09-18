
#include "clock_tm.hpp"
#include <sys/time.h>
#include "esp_log.h"
#include "utils/kvs.hpp"

namespace clock_tm
{
    static const char *TAG = "clock_tm";
    constexpr auto kvs_tz = "tz";

    clock::clock(cb_t &&cb) : minute_timer_([this, cb_ = std::move(cb)]()
                                            {
    time_t now;
    struct tm timeinfo;
    time(&now);

    char strftime_buf[64];

    localtime_r(&now, &timeinfo);
    minute_timer_.start(std::chrono::seconds(60 - timeinfo.tm_sec));

    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "Local time: %s", strftime_buf);    

    cb_(timeinfo); })
    {

        const auto tz = get_tz();
        ESP_LOGI(TAG, "TZ=%s", tz.c_str());
        setenv("TZ", tz.c_str(), 1);
        tzset();

        minute_timer_.start(std::chrono::seconds(1));
    }

    void update_time_zone(const std::string &tz)
    {
        auto kvss = kvs::handler(TAG);
        ESP_LOGI(TAG, "tz %s", tz.c_str());

        if (ESP_OK != kvss.set_value(kvs_tz, tz))
        {

            ESP_LOGE(TAG, "error wr");
        }
    }
    std::string get_tz()
    {
        auto kvss = kvs::handler(TAG);
        std::string tz;
        kvss.get_value_or(kvs_tz, tz, CONFIG_TIMEZONE);
        return tz;
    }
}
