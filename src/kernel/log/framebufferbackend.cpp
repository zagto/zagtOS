#include <log/framebufferbackend.hpp>
#include <log/framebufferfont.hpp>

using namespace log;
using namespace framebufferbackend;


void FramebufferBackend::writePixel(u32 x, u32 y, Color color) {
    volatile u8 *pixel = &framebuffer[y * bytesPerLine + x * bytesPerPixel];
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


void FramebufferBackend::clear(bool fuzzy) {
    for (u32 y = 0; y < height; y++) {
        for (u32 x = 0; x < width; x++) {
            if (!fuzzy) {
                writePixel(x, y, backgroundColor);
            } else if ((x % 2 + y) % 2) {
                writePixel(x, y, fuzzyColor);
            }
        }
    }
}


void FramebufferBackend::init(const BootInfo::FramebufferInfo *framebufferInfo) {
    framebuffer = reinterpret_cast<u8 *>(framebufferInfo->baseAddress);
    width = framebufferInfo->width;
    height = framebufferInfo->height;
    bytesPerLine = framebufferInfo->bytesPerLine;
    bytesPerPixel = framebufferInfo->bytesPerPixel;
    format = static_cast<FramebufferFormat>(framebufferInfo->format);

    numColumns = width / Font::characterWidth;
    numLines = height / Font::characterHeight;
    currentColumn = 0;
    currentLine = 0;

    clear();
}


void FramebufferBackend::newLine() {
    // TODO: check for end of screen
    currentColumn = 0;
    currentLine++;

    if (currentLine >= numLines) {
        currentLine = 0;
        clear(true);
    }
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

    for (u32 y = 0; y < Font::characterHeight; y++) {
        for (u32 x = 0; x < Font::characterWidth; x++) {
            writePixel(currentColumn * Font::characterWidth + x,
                       currentLine * Font::characterHeight + y,
                       Font::getPixel(character, x, y) ? foregroundColor : backgroundColor);
        }
    }
    increasePosition();
}
