/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#pragma once

#include <string>
#include <inttypes.h>
#include <vector>

namespace proto
{
    struct ldr_t
    {
        int max;
        int min;
    };

    bool get(const std::string &payload, ldr_t &data);
    std::string to_str(const ldr_t &data);

    struct display_t
    {
        uint8_t segment_rotation;
        bool segment_upsidedown;
        bool mirrored;
    };
    bool get(const std::string &payload, display_t &data);
    std::string to_str(const display_t &data);

    struct brightness_t
    {
        using point_t = std::pair<uint16_t, uint8_t>; // lighting,brightness
        std::vector<point_t> points;
    };

    bool get(const std::string &payload, brightness_t &data);
    std::string to_str(const brightness_t &data);

    struct timezone_t
    {
        std::string tz;
    };
    bool get(const std::string &payload, timezone_t &data);
    std::string to_str(const timezone_t &data);

    struct mqtt_t
    {
        std::string url;
    };
    bool get(const std::string &payload, mqtt_t &data);
    std::string to_str(const mqtt_t &data);

} // namespace proto
