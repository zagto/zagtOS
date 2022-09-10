#pragma once

#include <common/inttypes.hpp>

class Font {
public:
    static const uint32_t characterWidth = 8;
    static const uint32_t characterHeight = 12;

    static bool getPixel(char character, uint32_t x, uint32_t y);
};

