/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#pragma once

#include <functional>
#include <string>
#include "esp_netif_ip_addr.h"

namespace utils {
std::string num_to_hex_string(const uint8_t* input, size_t size, char separator = 0);
std::string get_mac();
std::string to_Str(const esp_ip4_addr_t& ip);

template<typename T>
class generic_sensor {
 public:
    using cb_t = std::function<void(const T&)>;
    generic_sensor(cb_t&& cb)
        : cb_(std::move(cb)) {}

 protected:
    void set(const T& val) {
        cb_(val);
    }

 private:
    cb_t cb_;
};

} // namespace utils
