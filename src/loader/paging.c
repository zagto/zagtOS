#include <efi.h>
#include <util.h>
#include <physical-memory.h>
#include <virtual-memory.h>
#include <paging.h>


PageTable *PagingContext = NULL;

#define PAGE_PRESENT        0x0000000000000001
#define PAGE_WRITEABLE      0x0000000000000002
#define PAGE_USER           0x0000000000000004
#define PAGE_LARGE_2MIB     0x0000000000000080
#define PAGE_GLOBAL         0x0000000000000100
#define PAGE_NON_EXECUTABLE 0x8000000000000000

#define PAGE_ADDRESS_MASK   0x000ffffffffff000

#define NUM_PAGE_TABLE_LEVELS 4


static void clearPageTable(PageTable *pageTable) {
    UINTN index;
    for (index = 0; index < ENTRIES_PER_PAGE_TABLE; index++) {
        (*pageTable)[index] = 0;
    }
}


static void createGlobalMasterPageTableEntries(void) {
    // upper half is global area
    PageTable *newPageTable;
    UINTN index;
    for (index = ENTRIES_PER_PAGE_TABLE; index < ENTRIES_PER_PAGE_TABLE; index++) {
        newPageTable = (PageTable *)AllocatePhysicalFrame();
        clearPageTable(newPageTable);
        (*PagingContext)[index] = (UINTN)newPageTable | PAGE_PRESENT | PAGE_WRITEABLE;
    }
}


void InitPaging(void) {
    PagingContext = (PageTable *)AllocatePhysicalFrame();
    Log("Creating Master Page Table at: ");
    LogUINTN((UINTN)PagingContext);
    Log("\n");
    clearPageTable(PagingContext);
    createGlobalMasterPageTableEntries();
}


void MapLoaderMemory(struct EfiMemoryMapInfo *mapInfo) {
    EFI_MEMORY_DESCRIPTOR *descriptor;
    UINTN frameIndex;

    for (descriptor = EfiMemoryMapGetFirstReclaimableEntry(mapInfo);
         descriptor != NULL;
         descriptor = EfiMemoryMapGetNextReclaimableEntry(mapInfo)) {
        for (frameIndex = 0; frameIndex < descriptor->NumberOfPages; frameIndex++) {
            MapAddress(descriptor->PhysicalStart + frameIndex * PAGE_SIZE,
                       descriptor->PhysicalStart + frameIndex * PAGE_SIZE,
                       TRUE,
                       TRUE,
                       FALSE);
        }
    }
}


void MapFramebufferMemory(struct FramebufferInfo *framebufferInfo) {
    UINTN framebufferOffset = framebufferInfo->baseAddress % PAGE_SIZE;
    UINTN firstAddress = framebufferInfo->baseAddress - framebufferOffset;
    UINTN lastAddress = framebufferInfo->baseAddress + framebufferInfo->height * framebufferInfo->bytesPerLine;
    UINTN numPages = (lastAddress - firstAddress - 1) / PAGE_SIZE + 1;
    UINTN index;

    for (index = 0; index < numPages; index++) {
        MapAddress(firstAddress + index * PAGE_SIZE,
                   FramebufferRegion.start + index * PAGE_SIZE,
                   TRUE,
                   FALSE,
                   FALSE);
    }

    /* update framebuffer info to contain the "new" address, as this structure will be passed to the
     * kernel later on */
    framebufferInfo->baseAddress = FramebufferRegion.start + framebufferOffset;
}


void CreateIdentityMap(EFI_PHYSICAL_ADDRESS maxPhysicalAddress) {
    EFI_PHYSICAL_ADDRESS addr;
    for (addr = 0; addr < maxPhysicalAddress; addr += 2 * 1024 * 1024) { /* 2MiB */
        MapAddress(addr, addr + IDENTITY_MAPPING_BASE, TRUE, FALSE, TRUE);
    }
}


#define ADDRESS_PAGE_SHIFT 12
#define PAGE_LEVEL_SHIFT 9
#define ADDRESS_PART_MASK ((1 << PAGE_LEVEL_SHIFT) - 1)
static PageTableEntry *getPageTableEntry(PageTable *pageTable, UINTN virtualAddress, UINTN level) {
    UINTN index = (virtualAddress >> (ADDRESS_PAGE_SHIFT + level * PAGE_LEVEL_SHIFT))
            & (ENTRIES_PER_PAGE_TABLE - 1);
    return &(*pageTable)[index];
}


void MapAddress(EFI_PHYSICAL_ADDRESS physicalAddress,
                EFI_VIRTUAL_ADDRESS virtualAddress,
                BOOLEAN writeable,
                BOOLEAN executable,
                BOOLEAN large) {
    UINTN level = NUM_PAGE_TABLE_LEVELS - 1;
    PageTable *pageTable = PagingContext;
    PageTableEntry *entry;
    PageTable *newPageTable;

    UINTN targetLevel = large ? 1 : 0;

    while (level > targetLevel) {
        entry = getPageTableEntry(pageTable, virtualAddress, level);

        if (!(*entry & PAGE_PRESENT)) {
            newPageTable = (PageTable *)AllocatePhysicalFrame();
            clearPageTable(newPageTable);
            *entry = (UINTN)newPageTable | PAGE_PRESENT | PAGE_WRITEABLE;
        }
        pageTable = (PageTable *)(*entry & PAGE_ADDRESS_MASK);
        level--;
    }

    entry = getPageTableEntry(pageTable, virtualAddress, targetLevel);
    if (*entry & PAGE_PRESENT) {
        Log("Attempt to map page ");
        LogUINTN(virtualAddress);
        Log(" to ");
        LogUINTN(physicalAddress);
        Log(" that is already mapped to");
        LogUINTN(*entry & PAGE_ADDRESS_MASK);
        Log("\n");
        Halt();
    }

    *entry = physicalAddress | PAGE_PRESENT;
    if (writeable) {
        *entry |= PAGE_WRITEABLE;
    }
    if (!executable) {
        *entry |= PAGE_NON_EXECUTABLE;
    }
    if (virtualAddress >= 0xffff800000000000) {
        *entry |= PAGE_GLOBAL;
    }
    if (large) {
        *entry |= PAGE_LARGE_2MIB;
    }
}
