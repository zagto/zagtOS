#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <efi.h>

#define FRAMEBUFFER_FORMAT_RGB 1
#define FRAMEBUFFER_FORMAT_BGR 2

struct FramebufferInfo {
    UINTN baseAddress;
    UINT32 width;
    UINT32 height;
    UINT32 bytesPerPixel;
    UINT32 bytesPerLine;
    UINT32 format;
};

struct FramebufferInfo *InitFramebuffer(void);

#endif // FRAMEBUFFER_H
