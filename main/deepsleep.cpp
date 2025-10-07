#include "deepsleep.hpp"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer_cxx.hpp"
#include "sdkconfig.h"
#include <bits/chrono.h>
#include <memory>

namespace deepsleep {
static const char* TAG = "SLEEP";

RTC_DATA_ATTR int bootCount = 0;
static std::unique_ptr<idf::esp_timer::ESPTimer> timeout_timer;

int get_boot_count() {
    return bootCount;
}

void deep_sleep(const std::chrono::milliseconds duration)
{

    ESP_LOGI(TAG, "boot count %d, sleep for %lldms", get_boot_count(),
        duration.count());
#ifdef CONFIG_DEEP_SLEEP_ENABLED
    esp_deep_sleep(std::chrono::duration_cast<std::chrono::microseconds>(duration).count());
#else
    ESP_LOGW(TAG, "Deep sleep disabled, not going to sleep");
#endif
}

void set_timeout(const std::chrono::milliseconds duration, const std::chrono::milliseconds sleep_duration)
{
    ESP_LOGI(TAG, "set timeout for %lldms, sleep duartion %lldms", duration.count(), sleep_duration.count());

    timeout_timer = std::make_unique<idf::esp_timer::ESPTimer>([sleep_duration]()
                                                               {
        ESP_LOGI(TAG, "timeout reached, entering deep sleep for %lldms",sleep_duration.count());
        deep_sleep(sleep_duration); });

    timeout_timer->start(duration);
}

void extend_timeout(const std::chrono::milliseconds duration){
    if (timeout_timer) {
        timeout_timer->start(duration);
        ESP_LOGI(TAG, "timeout extended to %lldms", duration.count());
    }
}

} // namespace deepsleep

void RTC_IRAM_ATTR esp_wake_deep_sleep() {
    esp_default_wake_deep_sleep();
    deepsleep::bootCount++;
}
