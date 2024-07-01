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

namespace mqtt = idf::mqtt;

constexpr auto *TAG = "MQTT";

MyClient::MyClient(const idf::mqtt::BrokerConfiguration &broker, const idf::mqtt::ClientCredentials &credentials, const idf::mqtt::Configuration &config):
  idf::mqtt::Client(broker,credentials,config)
  { 
	esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
    }

void MyClient::on_connected(esp_mqtt_event_handle_t const event) 
{
    using mqtt::QoS;
    subscribe(messages.get());
    subscribe(sent_load.get(), QoS::AtMostOnce);
}
void MyClient::on_data(esp_mqtt_event_handle_t const event) 
{
    if (messages.match(event->topic, event->topic_len)) {
        ESP_LOGI(TAG, "Received in the messages topic");
    }
}


void mqtt_main()
{
    mqtt::BrokerConfiguration broker{
        .address = {mqtt::URI{std::string{CONFIG_BROKER_URL}}},
        .security =  mqtt::Insecure{}
    };
    mqtt::ClientCredentials credentials{};
    mqtt::Configuration config{};

    MyClient client{broker, credentials, config};
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
