#include "deepsleep.hpp"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include <bits/chrono.h>

namespace deepsleep {
static const char* TAG = "SLEEP";

RTC_DATA_ATTR int bootCount = 0;

int get_boot_count() {
    return bootCount;
}

void deep_sleep(const std::chrono::microseconds duration) {
    ESP_LOGI(TAG, "boot count %d, sleep for %lldms", get_boot_count(),
        std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    esp_deep_sleep(duration.count());
}
} // namespace deepsleep

void RTC_IRAM_ATTR esp_wake_deep_sleep() {
    esp_default_wake_deep_sleep();
    deepsleep::bootCount++;
}
