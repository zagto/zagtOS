#ifndef FRAMEBUFFERBACKEND_HPP
#define FRAMEBUFFERBACKEND_HPP

#include <setup/BootInfo.hpp>

namespace log {
    namespace framebufferbackend {
        enum class FramebufferFormat {
            RGB = 1,
            BGR = 2
        };

        struct Color {
            u8 red;
            u8 green;
            u8 blue;
        };

        class FramebufferBackend
        {
        private:
            constexpr static const Color backgroundColor{0x24, 0xb6, 0xb6};
            constexpr static const Color foregroundColor{0xff, 0xcc, 0x00};
            constexpr static const Color fuzzyColor{0x88, 0x00, 0x00};

            volatile u8 *framebuffer;
            u32 width;
            u32 height;
            u32 bytesPerLine;
            u32 bytesPerPixel;
            FramebufferFormat format;
            u32 numColumns;
            u32 numLines;
            u32 currentColumn;
            u32 currentLine;

            void clear(bool fuzzy = false);
            void writePixel(u32 x, u32 y, Color color);
            void newLine();
            void increasePosition();

        public:
            void init(const BootInfo::FramebufferInfo *framebufferInfo);
            void write(char character);
        };
    }

    using framebufferbackend::FramebufferBackend;
}

#endif // FRAMEBUFFERBACKEND_HPP
