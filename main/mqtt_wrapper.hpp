/*
 * mqtt.h
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#pragma once
#include <memory>
#include <string.h>
#include <chrono>
#include <queue>
#include <functional>
#include "esp_mqtt.hpp"
#include "esp_mqtt_client_config.hpp"

namespace mqtt {
class CMQTTWrapper: public idf::mqtt::Client {
 private:
    using msg_queue_t = struct {
        std::string topic;
        std::string msg;
    };
    using on_connect_cb_t = std::function<void()>;
    std::queue<msg_queue_t> send_queue_;
    bool                    is_connected_ = false;
    EventGroupHandle_t      event_group_;
    on_connect_cb_t         on_connect_cb_;

 public:
    CMQTTWrapper(on_connect_cb_t&& cb);
    virtual ~CMQTTWrapper();
    void publish(const std::string& topic, const std::string& message);
    bool flush(const std::chrono::milliseconds timeout);

 private:
    void on_connected(const esp_mqtt_event_handle_t event) final;
    void on_disconnected(const esp_mqtt_event_handle_t event) final;
    void on_published(const esp_mqtt_event_handle_t event) final;
    void on_data(const esp_mqtt_event_handle_t event) final{};

    void send_queue();
};

} // namespace mqtt
