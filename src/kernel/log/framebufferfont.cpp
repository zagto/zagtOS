#include <common/common.hpp>
#include <log/framebufferfont.hpp>
#include <log/beautifulfont.xbm>

using namespace log::framebufferbackend;


bool Font::getPixel(char character, u32 x, u32 y) {
    if (character < 32 || character > 126) {
        // always use the same character for stuff we can't print
        character = 0;
    }
    if (x >= characterWidth || y >= characterHeight) {
        // Invalid font access. Don't try to log this because this likely happened during logging
        Halt();
    }

    usize bit = y * beautifulfont_width + static_cast<u32>(character) * characterWidth + x;
    usize byte = bit / 8;
    usize bitInByte = bit % 8;
    return (beautifulfont_bits[byte] >> bitInByte) & 1;
}
