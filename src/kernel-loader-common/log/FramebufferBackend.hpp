#pragma once

#include <setup/HandOverState.hpp>

enum class FramebufferFormat {
    RGB = 1,
    BGR = 2
};

struct Color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

class FramebufferBackend {
private:
    constexpr static const Color backgroundColor{0xff, 0xff, 0xff};
    Color foregroundColor;

    uint8_t *frontBuffer;
    uint8_t *backBuffer;
    uint32_t width;
    uint32_t height;
    uint32_t bytesPerLine;
    uint32_t bytesPerPixel;
    FramebufferFormat format;
    uint32_t numColumns;
    uint32_t numLines;
    uint32_t currentColumn;
    uint32_t currentLine;

    void clear();
    void clearLine(uint32_t line);
    void writePixel(uint32_t x, uint32_t y, Color color);
    void newLine();
    void increasePosition();
    void flip();
    void writeCharacter(char character);
    void setKernelColor();
    void setProgramNameColor();
    void setProgramColor();

public:
    void init(const hos_v1::FramebufferInfo &framebufferInfo);
    void write(char character);
};
