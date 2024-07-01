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
#include "driver/gpio.h"
#include "esp_chip_info.h"
#include "esp_flash.h"

#include "esp_system.h"

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>

#include "provision.h"
#include "mqtt.hpp"

#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include <chrono>

#include "esp_exception.hpp"
#include <iostream>
#include "esp_err.h"
#include "esp_event_cxx.hpp"
#include "esp_event.h"
#include "memory"

std::shared_ptr<idf::event::ESPEventLoop> el;

static const char* TAG        = "app";
constexpr auto     BLINK_GPIO = GPIO_NUM_8;

static QueueHandle_t led_queue_handle;
static TaskHandle_t  led_task_handle   = NULL;
static bool          is_task_suspended = false;

int  relayState      = 0;
int  ledState        = 0;
bool led_blink_state = true;

enum class led_messages_e {
    LED_BLINK_SLOW_START,
    LED_BLINK_SLOW_STOP,
    LED_BLINK_FAST_START,
    LED_BLINK_FAST_STOP,
    LED_BLINK_STOP
};

BaseType_t led_send_message(led_messages_e led_msgID) {
    //  return xQueueSend(led_queue_handle, &led_msgID, portMAX_DELAY);
    return {};
}

void set_led_state(bool arg) {
    led_blink_state = arg;
}

bool led_state(void) {
    return led_blink_state;
}

void led_blink_function(uint32_t duration) {
    while (led_blink_state) {
        led_blink_state = led_state();
        if (!led_blink_state) {
            break;
        }
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(duration / portTICK_PERIOD_MS);
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(duration / portTICK_PERIOD_MS);
    }
}

void suspend_led_task(void) {
    vTaskSuspend(led_task_handle);
    is_task_suspended = true;
}

void resume_led_task(void) {
    vTaskResume(led_task_handle);
    is_task_suspended = false;
}

void led_task(void*) {
    led_messages_e msg;
    //  uint32_t       blink_duration = 1000;

    //	led_send_message(LED_BLINK_FAST_START);

    while (1) {
        if (xQueueReceive(led_queue_handle, &msg, portMAX_DELAY)) {
            switch (msg) {
                case led_messages_e::LED_BLINK_SLOW_START:

                    ESP_LOGI(TAG, "LED_BLINK_SLOW_START");
                    led_blink_function(1000);
                    xQueueReset(led_queue_handle);
                    break;

                case led_messages_e::LED_BLINK_SLOW_STOP:
                    ESP_LOGI(TAG, "LED_BLINK_SLOW_STOP");
                    suspend_led_task();

                    break;

                case led_messages_e::LED_BLINK_FAST_START:

                    ESP_LOGI(TAG, "LED_BLINK_FAST_START");
                    led_blink_function(500);

                    break;

                case led_messages_e::LED_BLINK_FAST_STOP:
                    ESP_LOGI(TAG, "LED_BLINK_FAST_STOP");
                    led_blink_function(2000);
                    suspend_led_task();

                    break;

                case led_messages_e::LED_BLINK_STOP:
                    ESP_LOGI(TAG, "LED_BLINK_STOP");
                    suspend_led_task();

                    break;

                default:
                    ESP_LOGI(TAG, "unknown");
                    break;
            }
        }
    }
}

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

static void configure_led(void) {
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

void init() {
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

    // ESP_ERROR_CHECK(esp_event_loop_create_default());

    configure_led();
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "[APP] Startup..");
    init();
    print_info();
    provision_main();
    //   mqtt_main();
    //    std::unique_ptr<ESPEventReg> reg_1 = loop.register_event(TEMPLATE_EVENT_0, [](const ESPEvent& event, void*
    //    data) {
    //        std::cout << "received event: " << event.base << "/" << event.id;
    //        if (data) {
    //            std::cout << "; event data: " << static_cast<unsigned>(*static_cast<led_messages_e*>(data));
    //        };
    //    });

    // xTaskCreate(led_task, "led_task", 4096, NULL, 5, &led_task_handle);
    ESP_LOGI(TAG, "started");
    led_send_message(led_messages_e::LED_BLINK_FAST_START);
    led_send_message(led_messages_e::LED_BLINK_SLOW_START);
    //    {
    //        auto tmp = led_messages_e::LED_BLINK_FAST_START;
    //        loop.post_event_data(TEMPLATE_EVENT_0, tmp);
    //    }
    //    idf::esp_timer::ESPTimer timer([]() {
    //        ESP_LOGI(TAG, "timeout");
    //        led_send_message(led_messages_e::LED_BLINK_SLOW_START);
    //    });
    //    timer.start(std::chrono::seconds(5));
    while (1) {
        //        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        //        gpio_set_level(BLINK_GPIO, s_led_state);
        //        /* Toggle the LED state */
        //        s_led_state = !s_led_state;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
