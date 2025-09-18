#include <memory>
#include <stdio.h>
#include <inttypes.h>
#include <chrono>
#include <iostream>
#include <math.h>
#include <string>

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
#include "provision.h"
#include "mqtt/mqtt_wrapper.hpp"
#include "display/blink.hpp"
#include "time/sntp.hpp"
#include "time/clock_tm.hpp"
#include "iot_button.h"
#include "sensors/sensor_event.hpp"
#include "sensors/htu2x.hpp"
#include "sensors/lighting.hpp"
#include "display/screen.hpp"
#include "display/layers.hpp"
#include "display/tests.hpp"
#include "display/font.hpp"
#include "display/transformation.hpp"
#include "utils/kvs.hpp"
#include "utils/utils.hpp"
#include "utils/puller.hpp"
#include "proto/defines.hpp"
#include "proto/handler.hpp"
#include "proto/http_server.hpp"

using namespace std::chrono_literals;
static const char *TAG = "main";

constexpr auto DEVICE_SW = "CLOCK "__DATE__
                           " " __TIME__;
layers::layers display;

std::unique_ptr<mqtt::CMQTTWrapper> mqtt_mng = nullptr;
std::unique_ptr<clock_tm::clock> clock_ptr = nullptr;
std::unique_ptr<utils::puller<int>> rssi_ptr = nullptr;
button_handle_t btn_ptr = nullptr;

proto::handler commands;

static EventGroupHandle_t app_main_event_group;
constexpr int GOT_IP = BIT0;

template <typename T>
void mqtt_send_sensor(const std::string &field, T value)
{
    if (mqtt_mng)
    {
        mqtt_mng->publish_device_brunch(field, value);
    }
}

static void event_got_ip_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    mqtt::device_info_t device_info;
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    device_info.ip = utils::to_Str(event->ip_info.ip);

    ESP_LOGI(TAG, "Connected with IP Address: %s", device_info.ip.c_str());
    display.show(4, device_info.ip, screen::js_right);

    /* Signal main application to continue execution */
    xEventGroupSetBits(app_main_event_group, GOT_IP);

    device_info.sw = DEVICE_SW;
    device_info.mac = utils::get_mac();

    mqtt_mng = std::make_unique<mqtt::CMQTTWrapper>(device_info, [](auto msg)
                                                    { return commands.on_command(msg); });
    rssi_ptr = std::make_unique<utils::puller<int>>([](int &rssi)
                                                    { return ESP_OK == esp_wifi_sta_get_rssi(&rssi); },
                                                    60s * 5, [](int rssi)
                                                    {
                                                        ESP_LOGI(TAG, "RSSI: %d", rssi); 
                                                        mqtt_send_sensor("rssi", rssi); });

    sntp::start();
    blink::stop(blink::BLINK_CONNECTING);
}

static void event_lost_ip_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    blink::start(blink::BLINK_CONNECTING);
    ESP_LOGI(TAG, "event_lost_ip_handler");
    rssi_ptr.reset();
    mqtt_mng.reset();
}

#define BOOT_BUTTON_NUM 9
#define BUTTON_ACTIVE_LEVEL 0
static void button_event_cb(void *arg, void *data)
{
    ESP_LOGW(TAG, "REQ REPROVISION");
    display.show(10, "***");
    blink::start(blink::BLINK_FACTORY_RESET);
    ESP_ERROR_CHECK(provision_reset());
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
}

void mqtt_temperature(void * /*arg*/, esp_event_base_t /*event_base*/, int32_t /*event_id*/, void *event_data)
{
    const auto event = (sensor_event::temperature_t *)event_data;
    mqtt_send_sensor("temperature", event->val);
}

void mqtt_humidity(void * /*arg*/, esp_event_base_t /*event_base*/, int32_t /*event_id*/, void *event_data)
{
    const auto event = (sensor_event::humidity_t *)event_data;
    mqtt_send_sensor("humidity", event->val);
}

void mqtt_lighting(void * /*arg*/, esp_event_base_t /*event_base*/, int32_t /*event_id*/, void *event_data)
{
    const auto event = (sensor_event::lighting_t *)event_data;
    mqtt_send_sensor("lighting", event->val);
    mqtt_send_sensor("adc", event->raw);
}

void init()
{
    utils::print_info();
    // Create a default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    app_main_event_group = xEventGroupCreate();
    kvs::init();

    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_got_ip_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &event_lost_ip_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &event_lost_ip_handler, NULL));
    blink::init();
    screen::init();

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
    lighting::init();
    commands.add("ldr", [](auto payload)
                 {
                    proto::ldr_t data;
                    ESP_LOGI(TAG, "esp_restart %s",payload.value_or("non").c_str());
            if (payload && proto::get(payload.value(),data))
            {
                ESP_LOGI(TAG, "esp_restart");
                lighting::set_adc_min(data.min);
                lighting::set_adc_max(data.max);
            }
            data.max=lighting::get_adc_max();
            data.min=lighting::get_adc_min();
        return proto::to_str(data); }, "ldr {min,max}");

    commands.add("brightness", [](auto payload)
                 {

        proto::brightness_t data;
        if (payload && proto::get(payload.value(), data))
        {
            screen::set_config_brightness(data.points);
        }
        data.points=screen::get_config_brightness();
        return proto::to_str(data); }, R"("points":[{"lighting":1530,"brightness":10}]")");

    commands.add("display", [](auto payload)
                 {
        proto::display_t data;
        if (payload && proto::get(payload.value(), data))
        {
            screen::set_config(data.segment_rotation, data.segment_upsidedown, data.mirrored);
        }
        screen::get_config(data.segment_rotation,data.segment_upsidedown,data.mirrored)  ;

        return proto::to_str(data); }, "display {segment_rotation,segment_upsidedown,mirrored}");

    commands.add("restart", [](auto)
                 {
        ESP_LOGI(TAG, "esp_restart");
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
        return "restart"; });

    commands.add("factory_reset", [](auto)
                 {
        ESP_LOGI(TAG, "factory_reset");
        ESP_ERROR_CHECK(nvs_flash_erase());
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
        return "factory_reset"; });

    commands.add("timezone", [](auto payload)
                 {
        proto::timezone_t data;
        if (payload && proto::get(payload.value(), data))
        {
            clock_tm::update_time_zone(data.tz);
        }
        data.tz = clock_tm::get_tz();
        return proto::to_str(data); }, "{tz:...}");

    commands.add("mqtt", [](auto payload)
                 {
        proto::mqtt_t data;
        if (payload && proto::get(payload.value(), data))
        {
            mqtt::set_config(data.url);    
        }
        mqtt::get_config(data.url);
        return proto::to_str(data); }, "{url:...}");

    ESP_LOGI(TAG, "%s", commands.get_cmd_list().c_str());

    sntp::init([]()
               {
    if (!clock_ptr)
    {
    clock_ptr = std::make_unique<clock_tm::clock>([](auto timeinfo)
                                                    { display.show(5, [&timeinfo]()
                                                                    {
            char buffMin[6];
            const auto cnt=sprintf(buffMin, "%u:%02u", timeinfo.tm_hour, timeinfo.tm_min);
            ESP_LOGI(TAG, "clock %s",buffMin); 
            const auto image_0 = font::get(buffMin,0);
            const u_int8_t space=(8 * CONFIG_DISPLAY_SEGMENTS-image_0.size())/(cnt-1);//space beatween symvols
            const auto image = font::get(buffMin,std::min(space,(u_int8_t)3));
            return transformation::image2buff(image, screen::js_center, 0); }); });
    } });
    auto &server = http_server::server::get_instance();
    server.set_uri("/cmd", [](const std::string &payload)
                   {
            ESP_LOGI(TAG, "http_server::server::get_instance() %s", payload.c_str());
        return commands.on_command(payload); });
}

/************************************
 *
 */
// #define UNIT_TEST
#ifndef UNIT_TEST
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    init();
    //  screen::tests();

    //------------------------------
    blink::start(blink::BLINK_PROVISIONING);
    provision_main();
    ESP_LOGI(TAG, "started");
    blink::stop(blink::BLINK_PROVISIONING);
    //------------------------------

    blink::start(blink::BLINK_CONNECTING);
    xEventGroupWaitBits(app_main_event_group, GOT_IP, pdTRUE, pdTRUE, portMAX_DELAY);

    htu2x::init();
    //--------------------------------
    ESP_ERROR_CHECK(esp_event_handler_register(sensor_event::event, sensor_event::internall_temperature, &mqtt_temperature, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(sensor_event::event, sensor_event::internall_humidity, &mqtt_humidity, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(sensor_event::event, sensor_event::lighting, &mqtt_lighting, NULL));

    ESP_LOGI(TAG, "done");
}

#else

#include "unity.h"
void do_tests_utils();

extern "C" void app_main(void)
{
    do_tests_utils();
}
#endif