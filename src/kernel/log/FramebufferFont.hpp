#ifndef FRAMEBUFFERFONT_HPP
#define FRAMEBUFFERFONT_HPP

#include <common/common.hpp>

class Font {
public:
    static const uint32_t characterWidth = 20;
    static const uint32_t characterHeight = 30;

    static bool getPixel(char character, uint32_t x, uint32_t y);
};

#endif // FRAMEBUFFERFONT_HPP
