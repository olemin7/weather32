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
#include "esp_mqtt.hpp"
#include "esp_mqtt_client_config.hpp"

namespace mqtt {
class CMQTTManager {
 public:
    CMQTTManager();
    ~CMQTTManager();
    void init();
    void publish(const std::string& topic, const std::string& message);
    bool flush(const std::chrono::milliseconds timeout);

 private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mqtt
