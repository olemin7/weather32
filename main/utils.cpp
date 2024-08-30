/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#include "utils.hpp"
#include "esp_mac.h"
#include <string>
#include <sstream>

namespace utils {
std::string num_to_hex_string(const uint8_t* input, size_t size, char separator) {
    static char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E',
        'F' };
    std::string       res;
    while (size--) {
        res += hex_chars[*input >> 4];
        res += hex_chars[*input & 0x0f];
        input++;
        if (separator && size) {
            res += separator;
        }
    }

    return res;
}

std::string get_mac() {
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));
    return num_to_hex_string(mac, sizeof(mac));
}

std::string to_Str(const esp_ip4_addr_t& ip) {
    auto              ipp = reinterpret_cast<const uint8_t*>(&ip);
    auto              sz  = 4;
    std::stringstream sstream;
    while (sz--) {
        sstream << static_cast<unsigned>(*ipp);
        ipp++;
        if (sz) {
            sstream << ':';
        }
    }
    return sstream.str();
}

} // namespace utils
