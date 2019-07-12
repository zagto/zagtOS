#ifndef BOOTINFO_HPP
#define BOOTINFO_HPP

#include <common/common.hpp>
#include <memory/FrameStack.hpp>

struct BootInfo {
    struct FramebufferInfo {
        usize baseAddress;
        u32 width;
        u32 height;
        u32 bytesPerPixel;
        u32 bytesPerLine;
        u32 format;
    };

    struct InitialObjectsInfo {
        PhysicalAddress address;
        usize size;
    };

    FramebufferInfo framebufferInfo;
    FrameStack freshFrameStack;
    FrameStack usedFrameStack;
    InitialObjectsInfo initDataInfo;
    PhysicalAddress masterPageTable;
};

#endif // BOOTINFO_HPP
