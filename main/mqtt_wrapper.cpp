/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* C++ MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "mqtt_wrapper.hpp"
#include "nvs_flash.h"

#include "esp_log.h"
#include <memory>

#include "sdkconfig.h"

namespace mqtt {
namespace imqtt = idf::mqtt;

constexpr auto* TAG         = "MQTT";
constexpr int   EMPTY_QUEUE = BIT0;

CMQTTWrapper::CMQTTWrapper(on_connect_cb_t&& cb)
    : imqtt::Client(imqtt::BrokerConfiguration{ .address = { imqtt::URI{ std::string{ CONFIG_BROKER_URL } } },
                        .security                        = imqtt::Insecure{} },
          {}, { .connection = { .disable_auto_reconnect = true } })
    , event_group_(xEventGroupCreate())
    , on_connect_cb_(std::move(cb)) {
    ESP_LOGD(TAG, "mqtt_wrapper ctor");
    ESP_LOGI(TAG, "CONFIG_BROKER_URL %s", CONFIG_BROKER_URL);
};

CMQTTWrapper::~CMQTTWrapper() {
    ESP_LOGD(TAG, "mqtt_wrapper dtor");
    esp_mqtt_client_destroy(handler.get());
}

void CMQTTWrapper::on_published(const esp_mqtt_event_handle_t /*event*/) {
    ESP_LOGI(TAG, "on_published");
    send_queue_.pop();
    send_queue();
}

void CMQTTWrapper::on_connected(esp_mqtt_event_handle_t const /*event*/) {
    ESP_LOGI(TAG, "connected");
    is_connected_ = true;
    on_connect_cb_();
    send_queue();
}

void CMQTTWrapper::on_disconnected(const esp_mqtt_event_handle_t event) {
    ESP_LOGI(TAG, "disconnected");
    is_connected_ = false;
}

void CMQTTWrapper::publish(const std::string& topic, const std::string& message) {
    ESP_LOGI(TAG, "add topic:%s, msg:%s", topic.c_str(), message.c_str());
    xEventGroupClearBits(event_group_, EMPTY_QUEUE);
    send_queue_.push({ topic, message });
    send_queue();
}

bool CMQTTWrapper::flush(const std::chrono::milliseconds timeout) {
    ESP_LOGI(TAG, "flush tm=%lldms, queue=%d", timeout.count(), send_queue_.size());
    if (!send_queue_.empty()) {
        const TickType_t xTicksToWait = timeout.count() / portTICK_PERIOD_MS;
        if (!xEventGroupWaitBits(event_group_, EMPTY_QUEUE, pdTRUE, pdFALSE, xTicksToWait)) {
            return false;
        }
    }
    return true;
}

void CMQTTWrapper::send_queue() {
    ESP_LOGI(TAG, "send_queue sz=%d", send_queue_.size());
    if (send_queue_.empty()) {
        xEventGroupSetBits(event_group_, EMPTY_QUEUE);
    } else if (is_connected_) {
        imqtt::Client::publish(send_queue_.front().topic, imqtt::StringMessage(send_queue_.front().msg));
    }
}

} // namespace mqtt
