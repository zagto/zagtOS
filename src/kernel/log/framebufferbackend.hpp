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
            uint8_t red;
            uint8_t green;
            uint8_t blue;
        };

        class FramebufferBackend
        {
        private:
            constexpr static const Color backgroundColor{0x24, 0xb6, 0xb6};
            constexpr static const Color foregroundColor{0xff, 0xcc, 0x00};
            constexpr static const Color fuzzyColor{0x88, 0x00, 0x00};

            volatile uint8_t *framebuffer;
            uint32_t width;
            uint32_t height;
            uint32_t bytesPerLine;
            uint32_t bytesPerPixel;
            FramebufferFormat format;
            uint32_t numColumns;
            uint32_t numLines;
            uint32_t currentColumn;
            uint32_t currentLine;

            void clear(bool fuzzy = false);
            void writePixel(uint32_t x, uint32_t y, Color color);
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
