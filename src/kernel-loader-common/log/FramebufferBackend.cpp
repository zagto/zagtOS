#pragma GCC optimize("O3")

#include <log/FramebufferBackend.hpp>
#include <log/FramebufferFont.hpp>
#include <log/BasicLog.hpp>
#include <common/utils.cpp>

/* in the bootloader, we want to use the framebuffer before memory management is initialized. So we
 * can't allocate a back buffer of the exact size needed. Instead here is 50MB of memory to use. */
#ifdef ZAGTOS_LOADER
static constexpr size_t STATIC_BACK_BUFFER_SIZE = 0x1700000; //50*1024*1024;
static uint8_t staticBackBuffer[STATIC_BACK_BUFFER_SIZE];
#endif

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
    //pixel[3] = 0x00;
}


void FramebufferBackend::clear() {
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            writePixel(x, y, backgroundColor);
        }
    }
}


void FramebufferBackend::clearLine(uint32_t line) {
    for (uint32_t y = line * characterHeightPixels; y < (line + 1) * characterHeightPixels; y++) {
        for (uint32_t x = 0; x < width; x++) {
            writePixel(x, y, backgroundColor);
        }
    }
}


void FramebufferBackend::init(const hos_v1::FramebufferInfo &framebufferInfo) {
    height = framebufferInfo.height;
    width = framebufferInfo.width;
    bytesPerLine = framebufferInfo.bytesPerLine;
    bytesPerPixel = framebufferInfo.bytesPerPixel;
    scaleFactor = framebufferInfo.scaleFactor;
    frontBuffer = framebufferInfo.frontBuffer;
#ifdef ZAGTOS_LOADER
    backBuffer = staticBackBuffer;
    uint32_t maxHeight = STATIC_BACK_BUFFER_SIZE / bytesPerLine;
    height = min(height, maxHeight);
#else
    backBuffer = framebufferInfo.backBuffer;
#endif
    format = static_cast<FramebufferFormat>(framebufferInfo.format);

    characterWidthPixels = Font::characterWidth * scaleFactor;
    characterHeightPixels = Font::characterHeight * scaleFactor;
    numColumns = width / characterWidthPixels;
    numLines = height / characterHeightPixels;
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


void FramebufferBackend::writeCharacter(char character) {
    if (character == '\n') {
        newLine();
        return;
    }
    for (uint32_t y = 0; y < Font::characterHeight; y++) {
        for (uint32_t yPixel = 0; yPixel < scaleFactor; yPixel++) {
            for (uint32_t x = 0; x < Font::characterWidth; x++) {
                for (uint32_t xPixel = 0; xPixel < scaleFactor; xPixel++) {
                    writePixel((currentColumn * Font::characterWidth + x) * scaleFactor + xPixel,
                               (currentLine * Font::characterHeight + y) * scaleFactor + yPixel,
                               Font::getPixel(character, x, y) ? foregroundColor : backgroundColor);
                }
            }
        }
    }
    increasePosition();
}

void FramebufferBackend::flip() {
    size_t offset = currentLine * characterHeightPixels;
    memcpy(frontBuffer, backBuffer + offset * bytesPerLine, (height - offset) * bytesPerLine);
    memcpy(frontBuffer + (height - offset) * bytesPerLine, backBuffer, offset * bytesPerLine);
}

void FramebufferBackend::write(char character) {
    switch (character) {
    case basicLog::ControlCharacter::KERNEL_COLOR:
        foregroundColor = {0, 0, 255};
        break;
    case basicLog::ControlCharacter::PROGRAM_NAME_COLOR:
        foregroundColor = {255, 128, 128};
        break;
    case basicLog::ControlCharacter::PROGRAM_COLOR:
        foregroundColor = {0, 0, 0};
        break;
    default:
        writeCharacter(character);
    }
}
