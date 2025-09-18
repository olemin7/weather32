#include "transformation.hpp"
#include <algorithm>
#include "esp_log.h"
namespace transformation
{
    using namespace screen;
    static const char *TAG = "TRANFORMATION";
    // Function to reverse the bits of an 8-bit integer
    uint8_t reverse_bits(uint8_t num)
    {
        uint8_t reversed = 0;
        for (uint8_t i = 0; i < 8; i++)
        {
            // Shift reversed to the left to make space
            reversed <<= 1;

            // Add the least significant bit of num
            reversed |= (num & 1);

            // Shift num to the right to process the next bit
            num >>= 1;
        }
        return reversed;
    }

    uint8_t get_column(const buffer_t &buffer, const uint8_t segment, uint8_t column)
    {
        uint8_t res = 0;
        const auto start = segment * 8;
        const auto mask = 1 << column;
        for (uint8_t i = 0; i < 8; i++)
        {
            res <<= 1;
            if (buffer[start + i] & mask)
            {
                res |= 1;
            }
        }

        return res;
    }

    inline uint8_t get_row(const buffer_t &buffer, const uint8_t segment, uint8_t row)
    {
        return buffer[segment * 8 + row];
    }

    buffer_t buffer_by_segment_rotate(const buffer_t &in, uint8_t rotation, uint8_t segments)
    {
        if (0 == rotation || 3 < rotation)
        {
            return in;
        }
        buffer_t out;
        for (uint8_t segment = 0; segment < segments; segment++)
        {

            for (uint8_t row = 0; row < 8; row++)
            {
                uint8_t line;
                switch (rotation)
                {
                case 1:
                    line = get_column(in, segment, row);
                    break;
                case 2:
                    line = reverse_bits(get_row(in, segment, 7 - row));
                    break;
                case 3:
                    line = reverse_bits(get_column(in, segment, 7 - row));
                    break;
                }
                out[segment * 8 + row] = line;
            }
        }
        return out;
    }

    buffer_t get_test_buffer()
    {
        buffer_t buffer;
        uint8_t pos = 0;
        for (auto &line : buffer)
        {
            if (pos % 8)
            {
                line = (1 << (pos / 8)) | 1;
            }
            else
            {
                line = 0xff;
            }

            pos++;
        }

        return buffer;
    }

    buffer_t
    image2buff(const std::vector<uint8_t> &image, const justify_t justify, const uint8_t offset)
    {

        if (offset > image.size())
        {
            ESP_LOGE(TAG, "offset=%d (remains %d)", offset, image.size());
            return {};
        }
        buffer_t buffer{0};
        uint8_t remain = image.size() - offset;
        if (remain > sizeof(buffer_t))
        {
            remain = sizeof(buffer_t);
        }
        switch (justify)
        {
        case js_left:
            std::copy_n(image.cbegin() + offset, remain, buffer.begin());
            break;
        case js_right:
            std::copy_n(std::reverse_iterator(image.cend()), remain, std::reverse_iterator(buffer.end()));
            break;
        case js_center:
        {
            const auto start = (sizeof(buffer_t) - remain) / 2;
            std::copy_n(image.cbegin() + offset, remain, buffer.begin() + start);
        }
        break;
        default:
            ESP_LOGE(TAG, "unexpected justify =%d", static_cast<int>(justify));
            break;
        }

        return buffer;
    }
}
