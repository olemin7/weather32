#include <memory>
#include <stdio.h>
#include <inttypes.h>
#include <chrono>
#include <iostream>
#include <math.h>
#include <string>
#include <map>

#include "rom/rtc.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <esp_log.h>
#include <esp_wifi.h>
#include "freertos/queue.h"

#include "esp_exception.hpp"
#include "esp_err.h"
#include "esp_timer_cxx.hpp"
#include <esp_event.h>

#include "json_helper.hpp"
#include "provision.hpp"
#include "mqtt/mqtt_wrapper.hpp"
#include "blink.hpp"
#include "iot_button.h"
#include "sensors/bme680.hpp"
#include "sensors/bh1750.hpp"
#include "utils/kvs.hpp"
#include "utils/utils.hpp"
#include "deepsleep.hpp"

using namespace std::chrono_literals;
static const char *TAG = "main";

constexpr auto DEVICE_SW = "weather32 "__DATE__
                           " " __TIME__;

std::unique_ptr<mqtt::CMQTTWrapper> mqtt_mng = nullptr;
std::unique_ptr<bme680::sensor> bme680_p = nullptr;
std::unique_ptr<bh1750::sensor> bh1750_p = nullptr;
std::unique_ptr<idf::esp_timer::ESPTimer> sleep_timer = nullptr;
std::map<std::string, std::string> sensors_data = {};

static EventGroupHandle_t app_main_event_group;
constexpr int GOT_IP = BIT0;
constexpr int GOT_SENSOR_DATA = BIT1;
constexpr int GOT_LIGHTING_DATA = BIT2;
constexpr int GOT_BAT = BIT3;
constexpr int MQTT_EMPTY = BIT4;

template <typename T>
void collect_sensors_data(const std::string &field, T value)
{
    sensors_data[field] = std::to_string(value);
}

static void event_got_ip_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    mqtt::device_info_t device_info;
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    device_info.ip = utils::to_Str(event->ip_info.ip);

    ESP_LOGI(TAG, "Connected with IP Address: %s", device_info.ip.c_str());

    /* Signal main application to continue execution */
    xEventGroupSetBits(app_main_event_group, GOT_IP);

    device_info.sw = DEVICE_SW;
    device_info.mac = utils::get_mac();

    mqtt_mng = std::make_unique<mqtt::CMQTTWrapper>(device_info);
    int rssi = -1;
    if (ESP_OK == esp_wifi_sta_get_rssi(&rssi))
    {
        ESP_LOGI(TAG, "RSSI: %d", rssi);
        collect_sensors_data("rssi", rssi);
    }
    blink::stop(blink::BLINK_CONNECTING);
}

constexpr auto BOOT_BUTTON_NUM = GPIO_NUM_9;
#define BUTTON_ACTIVE_LEVEL 0
static void button_event_cb(void *arg, void *data)
{
    blink::init();
    ESP_LOGW(TAG, "REQ REPROVISION");
    blink::start(blink::BLINK_FACTORY_RESET);
    ESP_ERROR_CHECK(provision_reset());
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
}

void sensors_off()
{
    ESP_LOGI(TAG, "sensors_off");
    bme680_p.reset();
    bh1750_p.reset();
}

void shootdown()
{
    ESP_LOGI(TAG, "SHUTDOWN");
    sensors_off();
    ESP_LOGI(TAG, "entering deep sleep ");
    deepsleep::sleep(10s);
}

void init()
{
    utils::print_info();
    // Create a default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    app_main_event_group = xEventGroupCreate();
    kvs::init();
    ESP_ERROR_CHECK(i2cdev_init());
    bme680_p = std::make_unique<bme680::sensor>([](auto value)
                                                {

                                                    collect_sensors_data("temperature", value.temperature);
                                                    collect_sensors_data("humidity", value.humidity);
                                                    collect_sensors_data("pressure", value.pressure);
                                                    xEventGroupSetBits(app_main_event_group, GOT_SENSOR_DATA); },

                                                []()
                                                { xEventGroupSetBits(app_main_event_group, GOT_SENSOR_DATA); });
    bh1750_p = std::make_unique<bh1750::sensor>([](auto lux)
                                                {
                                                    collect_sensors_data("lux", lux);
                                                    xEventGroupSetBits(app_main_event_group, GOT_LIGHTING_DATA); },
                                                []()
                                                { xEventGroupSetBits(app_main_event_group, GOT_LIGHTING_DATA); });

    sleep_timer = std::make_unique<idf::esp_timer::ESPTimer>([]()
                                                             { shootdown(); });
    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_got_ip_handler, NULL));
    button_config_t btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 5 * 1000,
        .short_press_time = 0,
        .gpio_button_config = {
            .gpio_num = BOOT_BUTTON_NUM,
            .active_level = BUTTON_ACTIVE_LEVEL,
        },
    };
    button_handle_t btn_ptr = iot_button_create(&btn_cfg);
    assert(btn_ptr);
    ESP_ERROR_CHECK(iot_button_register_cb(btn_ptr, BUTTON_LONG_PRESS_START, button_event_cb, NULL));
}

/************************************
 *
 */
// #define UNIT_TEST
#ifndef UNIT_TEST
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Startup");
    init();
    //------------------------------

    provision_main();
    ESP_LOGI(TAG, "Started");
    sleep_timer->start(7s);
    //------------------------------

    blink::start(blink::BLINK_CONNECTING);
    xEventGroupWaitBits(app_main_event_group, GOT_SENSOR_DATA | GOT_LIGHTING_DATA /* | GOT_BAT*/, pdTRUE, pdTRUE, portMAX_DELAY);
    sensors_off();
    xEventGroupWaitBits(app_main_event_group, GOT_IP, pdTRUE, pdTRUE, portMAX_DELAY);

    if (mqtt_mng)
    {
        ESP_LOGI(TAG, "send sensors data");
        for (const auto &pair : sensors_data)
        {
            mqtt_mng->publish_device_brunch(pair.first, pair.second);
        }
    }

    mqtt_mng->is_all_send_cb([]()
                             { xEventGroupSetBits(app_main_event_group, MQTT_EMPTY); });
    xEventGroupWaitBits(app_main_event_group, MQTT_EMPTY, pdTRUE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "done");
    sleep_timer->stop();
    sleep_timer->start(0s);
}

#else

#include "unity.h"
void do_tests_utils();

extern "C" void app_main(void)
{
    do_tests_utils();
}
#endif
