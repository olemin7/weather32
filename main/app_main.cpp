/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <memory>
#include <stdio.h>
#include <inttypes.h>
#include <chrono>
#include <iostream>

#include "rom/rtc.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"

#include "esp_system.h"

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include "freertos/queue.h"

#include "esp_exception.hpp"

#include "esp_err.h"
#include "esp_event_cxx.hpp"
#include "esp_timer_cxx.hpp"

#include "json_helper.hpp"
#include "provision.h"
#include "mqtt.hpp"
#include "blink.hpp"
#include "sensors.hpp"
#include "deepsleep.hpp"
#include <math.h>

using namespace std::chrono_literals;

std::shared_ptr<idf::event::ESPEventLoop> el;
sensors::CManager                         sensors_mng;
mqtt::CMQTTManager                        mqtt_mng;
static const char*                        TAG = "APP";

static EventGroupHandle_t app_main_event_group;
constexpr int             SENSORS_DONE = BIT0;

void print_info() {
    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t        flash_size;
    ESP_LOGI(TAG, "Boot count %d", deepsleep::get_boot_count());
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "This is %s chip with %d CPU core(s), %s%s%s%s, ", CONFIG_IDF_TARGET, chip_info.cores,
        (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "", (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
        (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
        (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    ESP_LOGI(TAG, "silicon revision v%d.%d, ", major_rev, minor_rev);
    if (esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    ESP_LOGI(TAG, "%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
        (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    ESP_LOGI(TAG, "Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    ESP_LOGI(TAG, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());
}

void init() {
    app_main_event_group = xEventGroupCreate();
    /* Initialize NVS partition */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Initialize the event loop */
    el = std::make_shared<idf::event::ESPEventLoop>();

    blink::init();
    sensors_mng.init();
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "[APP] Startup..");
    print_info();
    init();
    provision_main();
    mqtt_mng.init();
    ESP_LOGI(TAG, "started");
    blink::set(blink::led_state_e::SLOW);

    auto json_obj = json::CreateObject();
    cJSON_AddStringToObject(json_obj.get(), "app_name", CONFIG_APP_NAME);
    cJSON_AddStringToObject(json_obj.get(), "ip", "xx:xx:xx:xx");
    cJSON_AddStringToObject(json_obj.get(), "mac", "xxxxxxxxx");
    mqtt_mng.publish(CONFIG_MQTT_TOPIC_ALIVE, PrintUnformatted(json_obj));
    sensors_mng.begin(
        [](const auto result) {
            ESP_LOGI(TAG, "result %d", static_cast<int>(result.status));

            auto sensors_obj = json::CreateObject();
            if (result.bme280) {
                AddFormatedToObject(sensors_obj, "temperature", "%.2f", result.bme280->temperature);
                AddFormatedToObject(sensors_obj, "humidity", "%.2f", result.bme280->humidity);
                AddFormatedToObject(sensors_obj, "pressure", "%.2f", result.bme280->pressure);
            }
            mqtt_mng.publish(CONFIG_MQTT_TOPIC_SENSORS, PrintUnformatted(sensors_obj));
            xEventGroupSetBits(app_main_event_group, SENSORS_DONE);
        },
        std::chrono::seconds(CONFIG_SENSORS_COLLECTION_TIMEOUT));

    xEventGroupWaitBits(app_main_event_group, SENSORS_DONE, pdTRUE, pdFALSE, 10000 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "wrapping");
    ESP_LOGI(TAG, "flush %d", mqtt_mng.flush(5s));
    deepsleep::deep_sleep(std::chrono::seconds(CONFIG_POOL_INTERVAL_DEFAULT));
}
