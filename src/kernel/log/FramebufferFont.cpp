#include <common/common.hpp>
#include <log/FramebufferFont.hpp>
#include <log/BeautifulFont.xbm>


bool Font::getPixel(char character, uint32_t x, uint32_t y) {
    if (character < 32 || character > 126) {
        // always use the same character for stuff we can't print
        character = 0;
    }
    if (x >= characterWidth || y >= characterHeight) {
        // Invalid font access. Don't try to log this because this likely happened during logging
        Halt();
    }

    size_t bit = y * beautifulfont_width + static_cast<uint32_t>(character) * characterWidth + x;
    size_t byte = bit / 8;
    size_t bitInByte = bit % 8;
    return (beautifulfont_bits[byte] >> bitInByte) & 1;
}