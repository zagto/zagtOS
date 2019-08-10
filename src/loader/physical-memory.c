#include <efi.h>
#include <util.h>
#include <efi-memory-map.h>
#include <framestack.h>
#include <physical-memory.h>
#include <paging.h>


struct FrameStack DirtyFrameStack = {
    .head = FRAMESTACK_NULL,
    .addIndex = 0,
};

struct FrameStack CleanFrameStack = {
    .head = FRAMESTACK_NULL,
    .addIndex = 0,
};

EFI_PHYSICAL_ADDRESS SecondaryProcessorEntry = 0;
static BOOLEAN secondaryProcessorEntryFound = FALSE;
static BOOLEAN masterPageTableFound = FALSE;

#define PROCESSOR_ENTRY_MAX (1ul << 16)
#define MASTER_PAGE_TABLE_MAX_MAX (1ul << 32)

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

            /* avoid re-using initdata memory for other stuff */
            if (address < initDataInfo->address
                    || address >= initDataInfo->address + initDataInfo->size) {

                /* MasterPageTable and SecondaryProcessorEntry have special physical location
                 * requirements to be usable in the real mode entry code.
                 * Reserve suitable frames for these */
                if (!secondaryProcessorEntryFound && address < PROCESSOR_ENTRY_MAX) {
                    SecondaryProcessorEntry = address;
                    secondaryProcessorEntryFound = TRUE;
                } else if (!masterPageTableFound && address < MASTER_PAGE_TABLE_MAX_MAX) {
                    MasterPageTable = (PageTable *)address;
                    masterPageTableFound = TRUE;
                } else {
                    FrameStackPush(&DirtyFrameStack, address);
                }
            }

            frameIndex++;
        }

        // prepare for next round
        descriptor = EfiMemoryMapGetNextAvailableEntry(mapInfo);
        frameIndex = 0;
    }

    if (!secondaryProcessorEntryFound) {
        Log("Unable to find frame for secondary processor entry code\n");
        Halt();
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
