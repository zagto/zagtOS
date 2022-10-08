#include <Framebuffer.hpp>
#include <memory/ArchRegions.hpp>
#include <iostream>
#include <setup/HandOverState.hpp>

static bool framebufferInitialized{false};
static hos_v1::FramebufferInfo info;

hos_v1::FramebufferInfo &InitFramebuffer(void) {
    info = hos_v1::FramebufferInfo{
        .frontBuffer = reinterpret_cast<uint8_t *>(0x9c000000) /* ARGB -> RGBA */,
        .backBuffer = nullptr, /* inserted by mapFrameBuffer */
        .width = 1080,
        .height = 2340,
        .bytesPerPixel = 4,
        .bytesPerLine = 1080*4,
        .format = hos_v1::FramebufferFormat::BGR,
        .scaleFactor = 3,
    };

    framebufferInitialized = true;
    return info;
}

hos_v1::FramebufferInfo &GetFramebuffer() {
    assert(framebufferInitialized);
    return info;
}
