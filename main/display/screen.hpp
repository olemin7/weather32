#pragma once
#include <esp_err.h>
#include <vector>
#include <array>
#include <stdint.h>
#include <string>
#include "sdkconfig.h"
/*
coordinate for seg4
(0,0)          (0,31)

(7,0)          (7,31)
*/
namespace screen
{
    using buffer_t = std::array<uint8_t, 8 * CONFIG_DISPLAY_SEGMENTS>;
    enum justify_t
    {
        js_left,
        js_center,
        js_right,
        js_fill,
    };

    void init();
    esp_err_t print(const buffer_t &buffer);
    esp_err_t print(const std::vector<uint8_t> &image, const justify_t justify = js_left, const uint8_t offset = 0);
    esp_err_t print(const std::string str, const justify_t justify = js_left, const uint8_t offset = 0);
    esp_err_t set_config(uint8_t segment_rotation,
                         bool segment_upsidedown,
                         bool mirrored);

    void get_config(uint8_t &segment_rotation,
                    bool &segment_upsidedown,
                    bool &mirrored);

    using brightness_point_t = std::pair<uint16_t, uint8_t>; // lighting,brightness
    esp_err_t set_config_brightness(const std::vector<brightness_point_t> &points);
    std::vector<brightness_point_t> get_config_brightness();
}; // namespace blink
