#ifndef BOOTINFO_H
#define BOOTINFO_H

#include <efi.h>
#include <framestack.h>
#include <framebuffer.h>

#define NUM_PRE_ALLOCALTED_FRAMES 16

struct InitDataInfo {
    UINTN address;
    UINTN size;
};

struct BootInfo {
    struct FramebufferInfo framebufferInfo;
    struct FrameStack freshFrameStack;
    struct FrameStack usedFrameStack;
    struct InitDataInfo initDataInfo;
    EFI_PHYSICAL_ADDRESS masterPageTable;
};

struct BootInfo *PrepareBootInfo(struct InitDataInfo *initDataInfo,
                                 const struct FramebufferInfo *framebufferInfo);

#endif // BOOTINFO_H
