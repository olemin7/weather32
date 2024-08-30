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
#include <math.h>

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
#include <string>
#include "freertos/queue.h"

#include "esp_exception.hpp"
#include "esp_wifi.h"
#include "esp_err.h"
#include "esp_event_cxx.hpp"
#include "esp_timer_cxx.hpp"

#include "json_helper.hpp"
#include "provision.h"
#include "mqtt_wrapper.hpp"
#include "blink.hpp"
#include "collector.hpp"
#include "deepsleep.hpp"
#include "utils.hpp"

using namespace std::chrono_literals;

std::shared_ptr<idf::event::ESPEventLoop> el;
std::unique_ptr<sensors::CCollector>      sensors_mng;
std::unique_ptr<mqtt::CMQTTWrapper>       mqtt_mng;
static const char*                        TAG = "APP";

static EventGroupHandle_t app_main_event_group;
constexpr int             SENSORS_DONE         = BIT0;
constexpr int             MQTT_CONNECTED_EVENT = BIT1;

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

static void event_got_ip_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
    ESP_LOGI(TAG, "Connected with IP Address: %s", utils::to_Str(event->ip_info.ip).c_str());
    /* Signal main application to continue execution */

    mqtt_mng =
        std::make_unique<mqtt::CMQTTWrapper>([]() { xEventGroupSetBits(app_main_event_group, MQTT_CONNECTED_EVENT); });
    auto json_obj = json::CreateObject();
    cJSON_AddStringToObject(json_obj.get(), "app_name", CONFIG_APP_NAME);
    cJSON_AddStringToObject(json_obj.get(), "ip", utils::to_Str(event->ip_info.ip).c_str());
    int rssi;
    ESP_ERROR_CHECK(esp_wifi_sta_get_rssi(&rssi));
    cJSON_AddNumberToObject(json_obj.get(), "rssi", rssi);

    cJSON_AddStringToObject(json_obj.get(), "mac", utils::get_mac().c_str());

    mqtt_mng->publish(CONFIG_MQTT_TOPIC_ADVERTISEMENT, PrintUnformatted(json_obj));
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
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_got_ip_handler, NULL));
    blink::init();
    sensors_mng =
        std::make_unique<sensors::CCollector>([](auto) { xEventGroupSetBits(app_main_event_group, SENSORS_DONE); });
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "[APP] Startup..");
    print_info();
    init();
    blink::set(blink::led_state_e::FAST);
    provision_main();
    ESP_LOGI(TAG, "started");
    const auto uxBits = xEventGroupWaitBits(app_main_event_group, SENSORS_DONE | MQTT_CONNECTED_EVENT, pdTRUE, pdTRUE,
        10000 / portTICK_PERIOD_MS);
    blink::set(blink::led_state_e::ON);
    ESP_LOGI(TAG, "wrapping");
    if (uxBits & MQTT_CONNECTED_EVENT) {
        const auto& sensors     = sensors_mng->get();
        auto        sensors_obj = json::CreateObject();
        if (sensors.bme280) {
            AddFormatedToObject(sensors_obj, "temperature", "%.2f", sensors.bme280->temperature);
            AddFormatedToObject(sensors_obj, "humidity", "%.2f", sensors.bme280->humidity);
            AddFormatedToObject(sensors_obj, "pressure", "%.2f", sensors.bme280->pressure);
        }
        const std::string topic = std::string(CONFIG_MQTT_TOPIC_SENSORS) + "/" + utils::get_mac();
        mqtt_mng->publish(topic.c_str(), PrintUnformatted(sensors_obj));

        ESP_LOGI(TAG, "flush %d", mqtt_mng->flush(5s));
    } else {
        ESP_LOGW(TAG, "no MQTT_CONNECTED_EVENT");
    }
    //  mqtt_mng.reset();some error
    sensors_mng.reset();
    blink::set(blink::led_state_e::OFF);
    deepsleep::deep_sleep(std::chrono::seconds(CONFIG_POOL_INTERVAL_DEFAULT));
}
