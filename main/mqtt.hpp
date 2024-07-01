/*
 * mqtt.h
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#pragma once
#include "esp_mqtt.hpp"
#include "esp_mqtt_client_config.hpp"

class MyClient  : public idf::mqtt::Client {
public:
  MyClient(const idf::mqtt::BrokerConfiguration &broker, const idf::mqtt::ClientCredentials &credentials, const idf::mqtt::Configuration &config);
 

  private:
  void              on_connected(esp_mqtt_event_handle_t const event) override;
  void              on_data(esp_mqtt_event_handle_t const event) override;
  idf::mqtt::Filter messages{ "$SYS/broker/messages/received" };
  idf::mqtt::Filter sent_load{ "$SYS/broker/load/+/sent" };
};

void mqtt_main();



