/*
 * mqtt.h
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#pragma once
#include <string.h>
#include <chrono>
#include <sstream>
#include <functional>
#include <vector>
#include "esp_mqtt.hpp"
#include "esp_mqtt_client_config.hpp"
#include <memory>
#include <set>
namespace mqtt
{
   struct device_info_t
   {
      std::string sw;
      std::string ip;
      std::string mac;
   };

   using command_cb_t = std::unique_ptr<std::function<std::string(const std::string &msg)>>;
   using all_send_cb_t = std::function<void(void)>;
   using connection_state_cb_t = std::function<void(bool)>;

   esp_err_t set_config(std::string url);
   void get_config(std::string &url);

   class CMQTTWrapper : public idf::mqtt::Client
   {
   private:
      using connection_state_cb_ptr_t = std::unique_ptr<connection_state_cb_t>;
      const device_info_t device_info_;
      idf::mqtt::Filter device_cmd_;
      idf::mqtt::Filter brodcast_cmd_;
      command_cb_t device_cmd_cb_;
      std::unique_ptr<all_send_cb_t> all_send_cb_;
      connection_state_cb_ptr_t connection_state_cb_;
      std::set<idf::mqtt::MessageID> send_mgs_list_;

   public:
      CMQTTWrapper(device_info_t &device_info, command_cb_t device_cmd_cb = nullptr, connection_state_cb_ptr_t connection_state_cb = nullptr);
      virtual ~CMQTTWrapper();
      void publish(const std::string &topic, const std::string &message);
      template <typename T>
      void publish(const std::string &topic, T value)
      {
         publish(topic, std::to_string(value));
      }

      template <typename T>
      void publish_device_brunch(const std::string &field, T value)
      {
         publish("devices/" + device_info_.mac + "/" + field, value);
      }
      bool is_all_send() const;
      void is_all_send_cb(all_send_cb_t &&);

   private:
      void on_connected(const esp_mqtt_event_handle_t event) final;
      void on_disconnected(const esp_mqtt_event_handle_t event) final;
      void on_published(const esp_mqtt_event_handle_t event) final;
      void on_data(const esp_mqtt_event_handle_t event) final;

      void send_advertisement();
   };

} // namespace mqtt
