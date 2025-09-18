/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#include "utils.hpp"
#include "esp_mac.h"
#include <string>
#include <sstream>
#include "esp_chip_info.h"
#include "esp_flash.h"
#include <esp_log.h>
#include "esp_system.h"

namespace utils {
    static const char *TAG = "utils";

    std::string num_to_hex_string(const uint8_t *input, size_t size, char separator)
    {
        static char const hex_chars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E',
                                           'F'};
        std::string res;
        while (size--)
        {
            res += hex_chars[*input >> 4];
            res += hex_chars[*input & 0x0f];
            input++;
            if (separator && size)
            {
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
            sstream << '.';
        }
    }
    return sstream.str();
}

void print_info()
{
    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "This is %s chip with %d CPU core(s), %s%s%s%s, ", CONFIG_IDF_TARGET, chip_info.cores,
             (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "", (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
             (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
             (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    ESP_LOGI(TAG, "silicon revision v%d.%d, ", major_rev, minor_rev);
    if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
    {
        printf("Get flash size failed");
        return;
    }

    ESP_LOGI(TAG, "%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    ESP_LOGI(TAG, "Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    ESP_LOGI(TAG, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());
}

float trimm(float in, uint8_t decimals)
{
    const auto mult = std::pow(10, decimals);
    const auto tmp = static_cast<int32_t>(in * mult);
    return static_cast<float>(tmp) / mult;
}

} // namespace utils
