/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#pragma once

#include <functional>
#include <string>
#include "esp_netif_ip_addr.h"
#include <cmath>
#include <vector>

namespace utils
{
    std::string num_to_hex_string(const uint8_t *input, size_t size, char separator = 0);
    std::string get_mac();
    std::string to_Str(const esp_ip4_addr_t &ip);
    void print_info();

    template <typename T>
    class generic_sensor
    {
    public:
        using cb_t = std::function<void(const T &)>;
        generic_sensor(cb_t &&cb)
            : cb_(std::move(cb)) {}

    protected:
        void set(const T &val)
        {
            cb_(val);
        }

    private:
        cb_t cb_;
    };

    float trimm(float in, uint8_t decimals);

    template <typename T>
    T to_range(const T in_min, const T in_max, T value)
    {
        const auto max = std::max(in_min, in_max);
        const auto min = std::min(in_min, in_max);
        if (value > max)
        {
            return max;
        }
        if (value < min)
        {
            return min;
        }
        return value;
    }

    template <typename T>
    T get_range(const T min, const T max)
    {
        if (max > min)
        {
            return max - min;
        }
        return min - max;
    }

    template <typename IN_T, typename OUT_T>
    OUT_T transform_range(const IN_T in_min, const IN_T in_max, const OUT_T out_min, const OUT_T out_max, IN_T value)
    {
        const auto val_in_range = to_range(in_min, in_max, value) - std::min(in_min, in_max);
        const auto range_in = get_range(in_max, in_min);
        const auto range_out = get_range(out_min, out_max);
        const auto offest_out = std::min(out_min, out_max);

        if ((in_max > in_min) ^ (out_max > out_min))
        { // invert
            return (range_in - val_in_range) * range_out / range_in + offest_out;
        }
        return val_in_range * range_out / range_in + offest_out;
    }

    // growing only
    template <typename IN_T, typename OUT_T>
    OUT_T transform_ranges(const std::vector<std::pair<IN_T, OUT_T>> &ranges, const IN_T value)
    {
        switch (ranges.size())
        {
        case 0:
            return static_cast<OUT_T>(value);

        case 1:
            return ranges.front().second;
        }

        auto imin = ranges.cbegin();
        auto imax = imin + 1;
        while ((imin->first < value) && (imax != ranges.cend()))
        {
            if (imax->first > value)
            {
                return transform_range(imin->first, imax->first, imin->second, imax->second, value);
            }
            imin = imax;
            imax++;
        }
        return imin->second;
    }

} // namespace utils
