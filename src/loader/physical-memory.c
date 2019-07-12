#include <efi.h>
#include <util.h>
#include <efi-memory-map.h>
#include <framestack.h>
#include <physical-memory.h>


struct FrameStack DirtyFrameStack = {
    .head = FRAMESTACK_NULL,
    .addIndex = 0,
};

struct FrameStack CleanFrameStack = {
    .head = FRAMESTACK_NULL,
    .addIndex = 0,
};

EFI_PHYSICAL_ADDRESS InitPhysicalFrameManagement(struct EfiMemoryMapInfo *mapInfo,
                                                 struct InitDataInfo *initDataInfo) {
    EFI_MEMORY_DESCRIPTOR *descriptor;
    EFI_PHYSICAL_ADDRESS maxPhysicalAddress = 0;
    UINTN frameIndex;

    descriptor = EfiMemoryMapGetFirstAvailableEntry(mapInfo);
    if (descriptor == NULL) {
        Log("No available Memory");
        Halt();
    }

    // The first available frame will be the first frame list frame
    DirtyFrameStack.head = (struct FrameStackNode *)descriptor->PhysicalStart;
    ClearFrame(DirtyFrameStack.head);
    DirtyFrameStack.head->next = FRAMESTACK_NULL;

    // start with 1 because 0 is our stack node frame, so it is not available
    frameIndex = 1;

    while (descriptor != NULL) {
        EFI_PHYSICAL_ADDRESS end = descriptor->PhysicalStart + descriptor->NumberOfPages * PAGE_SIZE;
        if (end > maxPhysicalAddress) {
            maxPhysicalAddress = end;
        }

        while (frameIndex < descriptor->NumberOfPages) {
            UINTN address = descriptor->PhysicalStart + frameIndex * PAGE_SIZE;

            if (address < initDataInfo->address
             || address >= initDataInfo->address + initDataInfo->size) {
                FrameStackPush(&DirtyFrameStack, address);
            }

            frameIndex++;
        }

        // prepare for next round
        descriptor = EfiMemoryMapGetNextAvailableEntry(mapInfo);
        frameIndex = 0;
    }

    CleanFrameStack.head = (struct FrameStackNode *)AllocatePhysicalFrame();
    ClearFrame(CleanFrameStack.head);
    CleanFrameStack.head->next = FRAMESTACK_NULL;

    // clean 100 frames so kernel can allocate memory before it's cleaning mechanism works
    for (UINTN i = 0; i < 100; i++) {
        void * frame = (void *)FrameStackPop(&DirtyFrameStack);
        ClearFrame(frame);
        FrameStackPush(&CleanFrameStack, (UINTN)frame);
    }
    return maxPhysicalAddress;
}


void *AllocatePhysicalFrame(void) {
    void *address = (void *)FrameStackPop(&DirtyFrameStack);
    ClearFrame(address);
    return address;
}


void FreePhysicalFrame(void *frame) {
    FrameStackPush(&DirtyFrameStack, (UINTN)frame);
}


void ClearFrame(void *frame) {
    for (UINTN index = 0; index < PAGE_SIZE; index++) {
        ((char *)frame)[index] = 0;
    }
}
