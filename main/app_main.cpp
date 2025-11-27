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

#include "utils/json_helper.hpp"
#include "provision/provision.hpp"
#include "mqtt_tools/mqtt_wrapper.hpp"
#include "display/blink.hpp"
#include "iot_button.h"
#include "libs/sensors/bme680.hpp"
#include "libs/sensors/bh1750.hpp"
#include "libs/sensors/adc.hpp"
#include "libs/utils/kvs.hpp"
#include "libs/utils/utils.hpp"
#include "libs/deepsleep.hpp"

using namespace std::chrono_literals;
static const char *TAG = "main";

constexpr auto DEVICE_SW = "weather32 "__DATE__
                           " " __TIME__;

std::unique_ptr<mqtt::CMQTTWrapper> mqtt_mng = nullptr;
std::unique_ptr<bme680::sensor> bme680_p = nullptr;
std::unique_ptr<bh1750::sensor> bh1750_p = nullptr;
std::unique_ptr<adc::sensor> adc_p = nullptr;
std::unique_ptr<idf::esp_timer::ESPTimer> sleep_timer = nullptr;
std::map<std::string, std::string> sensors_data = {};

static EventGroupHandle_t app_main_event_group;
constexpr int GOT_IP = BIT0;
constexpr int GOT_SENSOR_DATA = BIT1;
constexpr int GOT_LIGHTING_DATA = BIT2;
constexpr int GOT_BAT = BIT3;
constexpr int MQTT_EMPTY = BIT4;

auto deep_sleep_duration = std::chrono::seconds(CONFIG_DEEP_SLEEP_DURATION_S);

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

    mqtt_mng = std::make_unique<mqtt::CMQTTWrapper>(device_info, nullptr,
                                                    std::make_unique<mqtt::connection_state_cb_t>([](auto connected) {}));
    int rssi = -1;
    if (ESP_OK == esp_wifi_sta_get_rssi(&rssi))
    {
        ESP_LOGI(TAG, "RSSI: %d", rssi);
        collect_sensors_data("rssi", rssi);
    }
    blink::stop(blink::BLINK_CONNECTING);
}

void sensors_off()
{
    ESP_LOGI(TAG, "sensors_off");
    bme680_p.reset();
    bh1750_p.reset();
    adc_p.reset();
}

void shootdown()
{
    ESP_LOGI(TAG, "SHUTDOWN");
    sensors_off();
    ESP_LOGI(TAG, "entering deep sleep ");
    deepsleep::sleep(deep_sleep_duration);
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

    adc_p = std::make_unique<adc::sensor>([](auto value)
                                          { const auto persentage = utils::transform_range<decltype(value), uint8_t>( CONFIG_BATTERY_MIN,CONFIG_BATTERY_MAX,0,100,value);
        ESP_LOGI(TAG, "bat_persentage %d", persentage);
        collect_sensors_data("bat_persentage", persentage);
        collect_sensors_data("adc", value);
        if (CONFIG_DEEP_SLEEP_LOWBAT > persentage)
        {
            ESP_LOGW(TAG, "Low BAT, %u", persentage);
            deep_sleep_duration = std::chrono::seconds(CONFIG_DEEP_SLEEP_LOWBAT_DURATION_S);
        }
                                            xEventGroupSetBits(app_main_event_group, GOT_BAT); },
                                          []()
                                          { xEventGroupSetBits(app_main_event_group, GOT_BAT); });
    sleep_timer = std::make_unique<idf::esp_timer::ESPTimer>([]()
                                                             { 
                                                                ESP_LOGW(TAG, "Timeout");
                                                                shootdown(); });
    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_got_ip_handler, NULL));

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;        // No interrupt
    io_conf.mode = GPIO_MODE_INPUT;               // Set as input
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_1);  // Select the pin
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // No pull-down
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;      // Enable pull-up

    // Apply the configuration
    gpio_config(&io_conf);
    if (deepsleep::get_boot_count() == 0)
    {
        ESP_LOGW(TAG, "The firs boot");
        blink::init();
        if ((gpio_get_level(GPIO_NUM_1) == 0))
        {
            ESP_LOGW(TAG, "Service swith after power reset");
            blink::start(blink::BLINK_FACTORY_RESET);
            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            ESP_ERROR_CHECK(esp_wifi_init(&cfg));
            ESP_ERROR_CHECK(provision_reset());
        }
    }
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
    sleep_timer->start(std::chrono::seconds(CONFIG_DEEP_SLEEP_TIMEOUT_S));
    //------------------------------

    blink::start(blink::BLINK_CONNECTING);
    xEventGroupWaitBits(app_main_event_group, GOT_SENSOR_DATA | GOT_LIGHTING_DATA | GOT_BAT, pdTRUE, pdTRUE, portMAX_DELAY);
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
    mqtt_mng->publish_device_brunch("sleep", deep_sleep_duration.count());
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
