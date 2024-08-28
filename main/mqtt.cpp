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

#include "mqtt.hpp"
#include "nvs_flash.h"

#include "esp_log.h"
#include <memory>
#include <queue>
#include "sdkconfig.h"

namespace mqtt {
namespace imqtt = idf::mqtt;

constexpr auto* TAG = "MQTT";
constexpr int   EMPTY_QUEUE = BIT0;

class CMQTTManager::Impl: public imqtt::Client {
 public:
    Impl(const imqtt::BrokerConfiguration& broker, const imqtt::ClientCredentials& credentials,
        const imqtt::Configuration& config)
        : imqtt::Client(broker, credentials, config)
        , event_group_(xEventGroupCreate()) {
        /*  esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
            esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
            esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
            esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
            esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
            esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);*/
    }
    void publish(const std::string& topic, const std::string& message) {
        ESP_LOGI(TAG, "add topic:%s, msg:%s", topic.c_str(), message.c_str());
        send_queue_.push({ topic, message });
        send_queue();
    }

    bool flush(const std::chrono::milliseconds timeout) {
        ESP_LOGI(TAG, "flush tm=%lldms", timeout.count());
        if (send_queue_.size()) {
            const TickType_t xTicksToWait = timeout.count() / portTICK_PERIOD_MS;
            if (!xEventGroupWaitBits(event_group_, EMPTY_QUEUE, pdTRUE, pdFALSE, xTicksToWait)) {
                return false;
            }
        }
        return true;
    }

 private:
    using msg_queue_t = struct {
        std::string topic;
        std::string msg;
    };
    std::queue<msg_queue_t> send_queue_;
    bool                    is_connected_ = false;
    EventGroupHandle_t      event_group_;

    void send_queue() {
        ESP_LOGI(TAG, "send_queue sz=%d", send_queue_.size());
        if (send_queue_.empty()) {
            xEventGroupSetBits(event_group_, EMPTY_QUEUE);
        } else if (is_connected_) {
            imqtt::Client::publish(send_queue_.front().topic, imqtt::StringMessage(send_queue_.front().msg));
        }
    }

    void on_published(const esp_mqtt_event_handle_t /*event*/) final {
        ESP_LOGI(TAG, "on_published");
        send_queue_.pop();
        send_queue();
    }

    void on_connected(esp_mqtt_event_handle_t const /*event*/) final {
        ESP_LOGI(TAG, "connected");
        is_connected_ = true;
        send_queue();
    }

    void on_disconnected(const esp_mqtt_event_handle_t event) final {
        ESP_LOGI(TAG, "disconnected");
        is_connected_ = false;
    }

    void on_data(esp_mqtt_event_handle_t const /*event*/) final {}
};

CMQTTManager::CMQTTManager() {
    ESP_LOGD(TAG, "mqtt_wrapper construct");
};

CMQTTManager::~CMQTTManager() = default;

void CMQTTManager::init() {
    ESP_LOGI(TAG, "CONFIG_BROKER_URL %s", CONFIG_BROKER_URL);
    imqtt::BrokerConfiguration broker{ .address = { imqtt::URI{ std::string{ CONFIG_BROKER_URL } } },
        .security                               = imqtt::Insecure{} };
    imqtt::ClientCredentials   credentials{};
    imqtt::Configuration       config{};
    impl_ = std::make_unique<Impl>(broker, credentials, config);
}

void CMQTTManager::publish(const std::string& topic, const std::string& message) {
    impl_->publish(topic, message);
}

bool CMQTTManager::flush(const std::chrono::milliseconds timeout) {
    return impl_->flush(timeout);
}

} // namespace mqtt
