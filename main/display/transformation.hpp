#pragma once
#include <stdint.h>
#include <vector>
#include <array>
#include "screen.hpp"

namespace transformation
{

    uint8_t reverse_bits(uint8_t num);
    screen::buffer_t buffer_by_segment_rotate(const screen::buffer_t &in, uint8_t rotation, uint8_t segments = CONFIG_DISPLAY_SEGMENTS);
    screen::buffer_t get_test_buffer();

    screen::buffer_t
    image2buff(const std::vector<uint8_t> &image, const screen::justify_t justify, const uint8_t offset);
}
