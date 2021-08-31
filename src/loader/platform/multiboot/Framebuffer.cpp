#include <Multiboot.hpp>
#include <Framebuffer.hpp>
#include <memory/ArchRegions.hpp>
#include <log/Logger.hpp>
#include <setup/HandOverState.hpp>

static hos_v1::FramebufferInfo info;

hos_v1::FramebufferInfo &InitFramebuffer(void) {
    FramebufferTag *mboot = MultibootInfo->getTag<FramebufferTag>(0);
    assert(mboot != 0);
    assert(((size_t)mboot) != 0);

    if (mboot->framebufferType != 1) {
        cout << "Multiboot Framebuffer not in RGB graphical mode!" << endl;
        Panic();
    }

    info = hos_v1::FramebufferInfo{
        .frontBuffer = reinterpret_cast<uint8_t *>(mboot->address),
        .backBuffer = nullptr, /* inserted by mapFrameBuffer */
        .width = mboot->width,
        .height = mboot->height,
        .bytesPerPixel = mboot->bpp / 8u,
        .bytesPerLine = mboot->pitch,
        .format = 0 /* inserted below */
    };

    if (mboot->bpp >= 24 && mboot->bpp % 8 == 0
            && mboot->redFieldPosition == 0
            && mboot->redMaskSize == 8
            && mboot->greenFieldPosition == 8
            && mboot->greenMaskSize == 8
            && mboot->blueFieldPosition == 16
            && mboot->blueMaskSize == 8) {
        info.format = hos_v1::FramebufferFormat::RGB;
    } else if (mboot->bpp >= 24 && mboot->bpp % 8 == 0
           && mboot->redFieldPosition == 16
           && mboot->redMaskSize == 8
           && mboot->greenFieldPosition == 8
           && mboot->greenMaskSize == 8
           && mboot->blueFieldPosition == 0
           && mboot->blueMaskSize == 8) {
       info.format = hos_v1::FramebufferFormat::BGR;
    } else {
        cout << "Multiboot Framebuffer has format that is neither RGB-888 not BGR:" << endl
             << "bpp: " << static_cast<size_t>(mboot->bpp) << endl
             << "red position " << static_cast<size_t>(mboot->redFieldPosition)
             << " maskSize " << static_cast<size_t>(mboot->redMaskSize) << endl
             << "green position " << static_cast<size_t>(mboot->greenFieldPosition)
             << " maskSize " << static_cast<size_t>(mboot->greenMaskSize) << endl
             << "blue position " << static_cast<size_t>(mboot->blueFieldPosition)
             << " maskSize " << static_cast<size_t>(mboot->blueMaskSize);

        Panic();
    }

    return info;
}
