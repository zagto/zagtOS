#include <efi.h>
#include <bootinfo.h>
#include <physical-memory.h>
#include <virtual-memory.h>
#include <paging.h>

struct BootInfo *PrepareBootInfo(struct InitDataInfo *initDataInfo,
                                 const struct FramebufferInfo *framebufferInfo, EFI_PHYSICAL_ADDRESS ACPIRoot) {
    struct BootInfo *bootInfo = AllocatePhysicalFrame();
    //struct MemoryFrameListInfo *memInfo = &bootInfo->memoryFrameListInfo;

    // map page so it is readable after kernel entry
    MapAddress((EFI_PHYSICAL_ADDRESS)bootInfo, (EFI_PHYSICAL_ADDRESS)bootInfo, FALSE, FALSE, FALSE);

    bootInfo->initDataInfo = *initDataInfo;

    InitVirtualMemory();

    FrameStackPrepareForKernel(&DirtyFrameStack);
    FrameStackPrepareForKernel(&CleanFrameStack);

    bootInfo->usedFrameStack = DirtyFrameStack;
    bootInfo->freshFrameStack = CleanFrameStack;

    // copy framebuffer info
    bootInfo->framebufferInfo = *framebufferInfo;

    bootInfo->masterPageTable = (EFI_PHYSICAL_ADDRESS)MasterPageTable;
    bootInfo->ACPIRoot = ACPIRoot;

    return bootInfo;
}
