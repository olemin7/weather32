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
#include <esp_wifi.h>
#include <esp_log.h>
#include <memory>
#include "utils/kvs.hpp"

#include "sdkconfig.h"

namespace mqtt
{
    namespace imqtt = idf::mqtt;
    using idf::mqtt::QoS;

    constexpr auto *TAG = "MQTT";
    constexpr int EMPTY_QUEUE = BIT0;
    constexpr auto kvs_url = "url";

    esp_err_t set_config(std::string url)
    {
        auto kvss = kvs::handler(TAG);

        ESP_LOGI(TAG, "url %s", url.c_str());

        return kvss.set_value(kvs_url, url);
    }

    void get_config(std::string &url)
    {
        auto kvss = kvs::handler(TAG);
        kvss.get_value_or(kvs_url, url, CONFIG_BROKER_URL);
    }

    std::string get_config_url()
    {
        std::string url;
        get_config(url);
        return url;
    }

    CMQTTWrapper::CMQTTWrapper(device_info_t &device_info, command_cb_t &&device_cmd_cb)
        : imqtt::Client(imqtt::BrokerConfiguration{.address = {imqtt::URI{get_config_url()}},
                                                   .security = imqtt::Insecure{}},
                        {}, {.connection = {.disable_auto_reconnect = true}}),
          device_info_(device_info), device_cmd_cb_(device_cmd_cb),
          device_cmd_("cmd/" + device_info_.mac), brodcast_cmd_("cmd")
    {
        ESP_LOGI(TAG, "CONFIG_BROKER_URL %s", CONFIG_BROKER_URL);
        //    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    };

    void CMQTTWrapper::on_connected(esp_mqtt_event_handle_t const /*event*/)
    {
        ESP_LOGI(TAG, "connected");
        subscribe(device_cmd_.get(), QoS::ExactlyOnce);
        subscribe(brodcast_cmd_.get(), QoS::ExactlyOnce);
        send_advertisement();
    }

    void CMQTTWrapper::on_disconnected(const esp_mqtt_event_handle_t event)
    {
        ESP_LOGI(TAG, "disconnected");
    }

    void CMQTTWrapper::publish(const std::string &topic, const std::string &message)
    {
        ESP_LOGI(TAG, "add topic:%s, msg:%s", topic.c_str(), message.c_str());

        imqtt::Client::publish(topic, imqtt::StringMessage(message));
    }

    void CMQTTWrapper::on_data(const esp_mqtt_event_handle_t event)
    {
        const std::string msg(event->data, event->data_len);
        ESP_LOGD(TAG, "Rec:%s", msg.c_str());
        if (device_cmd_.match(event->topic, event->topic_len))
        {
            auto response = device_cmd_cb_(msg);
            if (response.length())
            {
                publish("response/" + device_info_.mac, R"({"payload":)" + response + "}");
            }
        }
        if (brodcast_cmd_.match(event->topic, event->topic_len))
        {
            if (msg == "adv")
            {
                send_advertisement();
            }
        }
    }

    void CMQTTWrapper::send_advertisement()
    {

        std::string info;

        info = "{\"sw\":\"" + device_info_.sw + "\",\"mac\":\"" + device_info_.mac + "\"}";

        publish(CONFIG_MQTT_TOPIC_ADVERTISEMENT, info);
        publish_device_brunch("ip", device_info_.ip);
    }

} // namespace mqtt
