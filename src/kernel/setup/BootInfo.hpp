#ifndef BOOTINFO_HPP
#define BOOTINFO_HPP

#include <common/common.hpp>
#include <memory/FrameStack.hpp>

struct BootInfo {
    struct FramebufferInfo {
        size_t baseAddress;
        uint32_t width;
        uint32_t height;
        uint32_t bytesPerPixel;
        uint32_t bytesPerLine;
        uint32_t format;
    };

    struct InitialObjectsInfo {
        PhysicalAddress address;
        size_t size;
    };

    FramebufferInfo framebufferInfo;
    FrameStack freshFrameStack;
    FrameStack usedFrameStack;
    InitialObjectsInfo initDataInfo;
    PhysicalAddress masterPageTable;
    PhysicalAddress ACPIRoot;
    PhysicalAddress secondaryProcessorEntry;
};

#endif // BOOTINFO_HPP
