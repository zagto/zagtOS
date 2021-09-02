#pragma GCC optimize("O3")

#include <log/FramebufferBackend.hpp>
#include <log/FramebufferFont.hpp>

void FramebufferBackend::writePixel(uint32_t x, uint32_t y, Color color) {
    volatile uint8_t *pixel = &backBuffer[y * bytesPerLine + x * bytesPerPixel];
    switch (format) {
    case FramebufferFormat::RGB:
        pixel[0] = color.red;
        pixel[1] = color.green;
        pixel[2] = color.blue;
        break;
    case FramebufferFormat::BGR:
        pixel[0] = color.blue;
        pixel[1] = color.green;
        pixel[2] = color.red;
        break;
    }
    pixel[3] = 0;
}


void FramebufferBackend::clear() {
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            writePixel(x, y, backgroundColor);
        }
    }
}


void FramebufferBackend::clearLine(uint32_t line) {
    for (uint32_t y = line * Font::characterHeight; y < (line + 1) * Font::characterHeight; y++) {
        for (uint32_t x = 0; x < width; x++) {
            writePixel(x, y, backgroundColor);
        }
    }
}


void FramebufferBackend::init(const hos_v1::FramebufferInfo &framebufferInfo) {
    frontBuffer = framebufferInfo.frontBuffer;
    backBuffer = framebufferInfo.backBuffer;
    width = framebufferInfo.width;
    height = framebufferInfo.height;
    bytesPerLine = framebufferInfo.bytesPerLine;
    bytesPerPixel = framebufferInfo.bytesPerPixel;
    format = static_cast<FramebufferFormat>(framebufferInfo.format);

    numColumns = width / Font::characterWidth;
    numLines = height / Font::characterHeight;
    currentColumn = 0;
    currentLine = 0;

    clear();
}


void FramebufferBackend::newLine() {
    currentColumn = 0;
    currentLine++;

    if (currentLine >= numLines) {
        currentLine = 0;
    }

    clearLine(currentLine);
    flip();
}


void FramebufferBackend::increasePosition() {
    currentColumn++;
    if (currentColumn >= numColumns) {
        newLine();
    }
}


void FramebufferBackend::write(char character) {
    if (character == '\n') {
        newLine();
        return;
    }
    for (uint32_t y = 0; y < Font::characterHeight; y++) {
        for (uint32_t x = 0; x < Font::characterWidth; x++) {
            writePixel(currentColumn * Font::characterWidth + x,
                       currentLine * Font::characterHeight + y,
                       Font::getPixel(character, x, y) ? foregroundColor : backgroundColor);
        }
    }
    increasePosition();
}

void FramebufferBackend::flip() {
    size_t offset = currentLine * Font::characterHeight;
    memcpy(frontBuffer, backBuffer + offset * bytesPerLine, (height - offset) * bytesPerLine);
    memcpy(frontBuffer + (height - offset) * bytesPerLine, backBuffer, offset * bytesPerLine);
}

void FramebufferBackend::setKernelColor() {
    foregroundColor = {0, 0, 255};
}

void FramebufferBackend::setProgramNameColor() {
    foregroundColor = {255, 128, 128};
}

void FramebufferBackend::setProgramColor() {
    foregroundColor = {0, 0, 0};
}
