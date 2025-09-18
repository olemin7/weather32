#pragma once
#include <stdint.h>
#include <vector>
#include <string>

namespace font
{
    const std::vector<uint8_t> get(std::string msg, uint8_t space = 1);
};