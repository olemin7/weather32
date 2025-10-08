#include "deepsleep.hpp"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"

namespace deepsleep {
static const char* TAG = "SLEEP";

RTC_DATA_ATTR int bootCount = 0;

int get_boot_count() {
    return bootCount;
}

void sleep(const std::chrono::milliseconds duration)
{

    ESP_LOGI(TAG, "boot count %d, sleep for %lldms", get_boot_count(),
        duration.count());
#ifdef CONFIG_DEEP_SLEEP_ENABLED
    esp_deep_sleep(std::chrono::duration_cast<std::chrono::microseconds>(duration).count());
#else
    ESP_LOGW(TAG, "Deep sleep disabled, not going to sleep");
#endif
}

} // namespace deepsleep

void RTC_IRAM_ATTR esp_wake_deep_sleep() {
    esp_default_wake_deep_sleep();
    deepsleep::bootCount++;
}
