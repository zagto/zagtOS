#include <Paging.hpp>
#include <memory/PhysicalMemory.hpp>
#include <log/Logger.hpp>

PageTable *HandOverMasterPageTable{nullptr};
PageTable *ProcessMasterPageTable{nullptr};

#define PAGE_PRESENT        0x0000000000000001
#define PAGE_WRITEABLE      0x0000000000000002
#define PAGE_USER           0x0000000000000004
#define PAGE_PAT_LOW        0x0000000000000008
#define PAGE_PAT_MID        0x0000000000000010
#define PAGE_LARGE_2MIB     0x0000000000000080
#define PAGE_GLOBAL         0x0000000000000100
#define PAGE_NON_EXECUTABLE 0x8000000000000000

#define PAGE_ADDRESS_MASK   0x000ffffffffff000

#define NUM_PAGE_TABLE_LEVELS 4


void ClearPageTable(PageTable *pageTable) {
    size_t index;
    for (index = 0; index < ENTRIES_PER_PAGE_TABLE; index++) {
        (*pageTable)[index] = 0;
    }
}


void CreateGlobalMasterPageTableEntries() {
    /* upper half is global area */
    PageTable *newPageTable;
    size_t index;
    for (index = ENTRIES_PER_PAGE_TABLE; index < ENTRIES_PER_PAGE_TABLE; index++) {
        newPageTable = reinterpret_cast<PageTable *>(AllocatePhysicalFrame().value());
        ClearPageTable(newPageTable);
        (*HandOverMasterPageTable)[index] =
                reinterpret_cast<uint64_t>(newPageTable) | PAGE_PRESENT | PAGE_WRITEABLE;
        (*ProcessMasterPageTable)[index] =
                reinterpret_cast<uint64_t>(newPageTable) | PAGE_PRESENT | PAGE_WRITEABLE;
    }
}


#define ADDRESS_PAGE_SHIFT 12
#define PAGE_LEVEL_SHIFT 9
#define ADDRESS_PART_MASK ((1 << PAGE_LEVEL_SHIFT) - 1)
static PageTableEntry *getPageTableEntry(PageTable *pageTable,
                                         VirtualAddress virtualAddress,
                                         size_t level) {
    size_t index = (virtualAddress.value() >> (ADDRESS_PAGE_SHIFT + level * PAGE_LEVEL_SHIFT))
            & (ENTRIES_PER_PAGE_TABLE - 1);
    return &(*pageTable)[index];
}


void MapAddress(PagingContext pagingContext,
                VirtualAddress virtualAddress,
                PhysicalAddress physicalAddress,
                bool writeable,
                bool executable,
                bool large,
                CacheType cacheType) {
    size_t level = NUM_PAGE_TABLE_LEVELS - 1;
    PageTable *pageTable;
    if (virtualAddress.isKernel()) {
        assert(pagingContext == PagingContext::GLOBAL);
        pageTable = HandOverMasterPageTable;
    } else if (pagingContext == PagingContext::HANDOVER) {
        pageTable = HandOverMasterPageTable;
    } else {
        assert(pagingContext == PagingContext::PROCESS);
        pageTable = ProcessMasterPageTable;
    }
    PageTableEntry *entry;
    PageTable *newPageTable;

    size_t targetLevel = large ? 1 : 0;

    while (level > targetLevel) {
        entry = getPageTableEntry(pageTable, virtualAddress, level);

        if (!(*entry & PAGE_PRESENT)) {
            newPageTable = reinterpret_cast<PageTable *>(AllocatePhysicalFrame().value());
            ClearPageTable(newPageTable);
            *entry = reinterpret_cast<uint64_t>(newPageTable) | PAGE_PRESENT | PAGE_WRITEABLE;
        }
        pageTable = (PageTable *)(*entry & PAGE_ADDRESS_MASK);
        level--;
    }

    entry = getPageTableEntry(pageTable, virtualAddress, targetLevel);
    if (*entry & PAGE_PRESENT) {
        cout << "Attempt to map page " << virtualAddress.value() << " to "
             << physicalAddress.value() << " that is already mapped to"
             << (*entry & PAGE_ADDRESS_MASK) << endl;
        Halt();
    }

    *entry = physicalAddress.value() | PAGE_PRESENT;
    if (writeable) {
        *entry |= PAGE_WRITEABLE;
    }
    if (!executable) {
        *entry |= PAGE_NON_EXECUTABLE;
    }
    if (virtualAddress.value() >= 0xffff800000000000) {
        *entry |= PAGE_GLOBAL;
    }
    if (large) {
        *entry |= PAGE_LARGE_2MIB;
    }
    if (cacheType) {
        *entry |= cacheType * PAGE_PAT_LOW;
    }
}
