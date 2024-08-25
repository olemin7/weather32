/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <memory>
#include <stdio.h>
#include <inttypes.h>
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
#include "freertos/event_groups.h"

#include <chrono>

#include "esp_exception.hpp"
#include <iostream>
#include "esp_err.h"
#include "esp_event_cxx.hpp"
#include "esp_timer_cxx.hpp"
#include <memory>

#include "provision.h"
#include "mqtt.hpp"
#include "blink.hpp"
#include "sensors.hpp"

using namespace std::chrono_literals;

std::shared_ptr<idf::event::ESPEventLoop> el;
sensors::CManager                         sensors_mng;
static const char*                        TAG = "app";

void print_info() {
    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t        flash_size;
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
    esp_log_level_set("Sensors", ESP_LOG_DEBUG);
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

ESP_EVENT_DEFINE_BASE(TEST_EVENT_BASE);
const idf::event::ESPEventID TEST_EVENT_ID_0(0);
const idf::event::ESPEventID TEST_EVENT_ID_1(1);

idf::event::ESPEvent TEMPLATE_EVENT_0(TEST_EVENT_BASE, TEST_EVENT_ID_0);
idf::event::ESPEvent TEMPLATE_EVENT_1(TEST_EVENT_BASE, TEST_EVENT_ID_1);

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "[APP] Startup..");
    init();
    print_info();
    provision_main();
    // mqtt_main();
    auto reg_1 = el->register_event(TEMPLATE_EVENT_0, [](const auto& event, auto* data) {
        std::cout << "received event: " << event.base << "/" << event.id;
        blink::set(blink::led_state_e::FAST);
    });

    // xTaskCreate(led_task, "led_task", 4096, NULL, 5, &led_task_handle);
    ESP_LOGI(TAG, "started");
    blink::set(blink::led_state_e::SLOW);
    sensors_mng.begin([](auto result) { ESP_LOGI(TAG, "result %d", static_cast<int>(result.status)); }, 5s);
    while (1) {
        //        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        //        gpio_set_level(BLINK_GPIO, s_led_state);
        //        /* Toggle the LED state */
        //        s_led_state = !s_led_state;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
