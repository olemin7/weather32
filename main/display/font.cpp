#include "font.hpp"
#include <map>
// https://xantorohara.github.io/led-matrix-editor/#7e1818181c181800|7e060c3060663c00|3c66603860663c00|30307e3234383000|3c6660603e067e00|3c66663e06663c00|1818183030667e00|3c66663c66663c00|3c66607c66663c00|3c66666e76663c00

const std::vector<uint8_t> glif_1 = {
    0b10000100,
    0b11111111,
    0b11111111,
    0b10000000,
};
const std::vector<uint8_t> glif_2 = {
    0b11000010,
    0b11100011,
    0b10110001,
    0b10011001,
    0b10001111,
    0b10000110,
};
const std::vector<uint8_t> glif_3 = {
    0b01000010,
    0b11000011,
    0b10001001,
    0b10001001,
    0b11111111,
    0b01110110,
};
const std::vector<uint8_t> glif_4 = {
    0b00111000,
    0b00101100,
    0b00100110,
    0b11111111,
    0b11111111,
    0b00100000,
};
const std::vector<uint8_t> glif_5 = {
    0b01001111,
    0b11001111,
    0b10000101,
    0b10000101,
    0b11111101,
    0b01111001,
};
const std::vector<uint8_t> glif_6 = {
    0b01111110,
    0b11111111,
    0b10001001,
    0b10001001,
    0b11111011,
    0b01110010,
};
const std::vector<uint8_t> glif_7 = {
    0b00000011,
    0b00000011,
    0b11110001,
    0b11111001,
    0b00001111,
    0b00000111,
};
const std::vector<uint8_t> glif_8 = {
    0b01110110,
    0b11111111,
    0b10001001,
    0b10001001,
    0b11111111,
    0b01110110,
};
const std::vector<uint8_t> glif_9 = {
    0b01001110,
    0b11011111,
    0b10010001,
    0b10010001,
    0b11111111,
    0b01111110,
};
const std::vector<uint8_t> glif_0 = {
    0b01111110,
    0b11111111,
    0b10000001,
    0b10000001,
    0b11111111,
    0b01111110,
};

const std::vector<uint8_t> glif_dot = {
    0b11000000,
    0b11000000,
};

const std::vector<uint8_t> glif_twodots = {
    0b01100110,
    0b01100110,
};

const std::vector<uint8_t> star = {
    0b00100100,
    0b00011000,
    0b01111110,
    0b00011000,
    0b00100100,
};

const std::vector<uint8_t> space = {
    0,
    0,
    0,
    0,
};

const auto table = std::map<char, const std::vector<uint8_t> &>{
    {'1', glif_1},
    {'2', glif_2},
    {'3', glif_3},
    {'4', glif_4},
    {'5', glif_5},
    {'6', glif_6},
    {'7', glif_7},
    {'8', glif_8},
    {'9', glif_9},
    {'0', glif_0},
    {'.', glif_dot},
    {':', glif_twodots},
    {'*', star},
    {' ', space},

};

namespace font
{
    const std::vector<uint8_t> &get(const char glif)
    {
        const auto it = table.find(glif);
        if (it != table.end())
        {
            return it->second;
        }
        return star;
    }

    const std::vector<uint8_t> get(std::string msg, uint8_t space)
    {
        std::vector<uint8_t> result;
        bool add_space = false;
        for (const auto &symbol : msg)
        {
            if (add_space && space)
            {
                auto count = space;
                while (count--)
                {
                    result.push_back(0);
                }
            }
            add_space = true;
            auto bitmap = get(symbol);
            std::copy(bitmap.begin(), bitmap.end(), std::back_inserter(result));
        }
        return result;
    }
};