#ifndef FRAMEBUFFERFONT_HPP
#define FRAMEBUFFERFONT_HPP

#include <common/common.hpp>

namespace log::framebufferbackend {
    class Font {
    public:
        static const u32 characterWidth = 20;
        static const u32 characterHeight = 30;

        static bool getPixel(char character, u32 x, u32 y);
    };
}

#endif // FRAMEBUFFERFONT_HPP
