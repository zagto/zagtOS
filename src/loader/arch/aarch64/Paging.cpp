#include <Paging.hpp>
#include <memory/PhysicalMemory.hpp>
#include <iostream>

PageTable *LowPagingRoot{nullptr};
PageTable *HighPagingRoot{nullptr};

constexpr uint64_t PAGE_PRESENT = 1;
/* not a block in terms of ARM paging, i.e. not a large page */
constexpr uint64_t PAGE_NOT_BLOCK = 1ul << 1;
constexpr uint64_t PAGE_ATTRIBUTES_INDEX_SHIFT = 2;
constexpr uint64_t PAGE_NON_SECURE = (1ul << 5);
constexpr uint64_t PAGE_USER = (1ul << 6);
constexpr uint64_t PAGE_READ_ONLY = (1ul << 7);
constexpr uint64_t PAGE_ACCESSED = (1ul << 10);
constexpr uint64_t PAGE_OUTER_SHARABLE = (0b10ul << 8);
constexpr uint64_t PAGE_INNER_SHARABLE = (0b11ul << 8);
constexpr uint64_t PAGE_NOT_GLOBAL = (1ul<<11);

constexpr uint64_t PAGE_NO_PRIVILEGED_EXECUTION = (1ul << 53);
constexpr uint64_t PAGE_NO_UNPRIVILEGED_EXECUTION = (1ul << 54);

constexpr uint64_t TABLE_NON_SECURE = (1ul << 63);


#define PAGE_ADDRESS_MASK   0x0007fffffffff000

#define NUM_PAGE_TABLE_LEVELS 3

void InitPaging() {
    LowPagingRoot = reinterpret_cast<PageTable *>(AllocatePhysicalFrame().value());
    HighPagingRoot = reinterpret_cast<PageTable *>(AllocatePhysicalFrame().value());
    ClearPageTable(LowPagingRoot);
    ClearPageTable(HighPagingRoot);
}

void ClearPageTable(PageTable *pageTable) {
    size_t index;
    for (index = 0; index < ENTRIES_PER_PAGE_TABLE; index++) {
        (*pageTable)[index] = 0;
    }
}

#define ADDRESS_PAGE_SHIFT 12
#define PAGE_LEVEL_SHIFT 9
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
        pageTable = HighPagingRoot;
    } else {
        assert(pagingContext == PagingContext::HANDOVER);
        pageTable = LowPagingRoot;
    }
    PageTableEntry *entry;
    PageTable *newPageTable;

    assert(!(writeable && executable));

    size_t targetLevel = large ? 1 : 0;

    while (level > targetLevel) {
        entry = getPageTableEntry(pageTable, virtualAddress, level);

        if (!(*entry & PAGE_PRESENT)) {
            newPageTable = reinterpret_cast<PageTable *>(AllocatePhysicalFrame().value());
            ClearPageTable(newPageTable);
            *entry = reinterpret_cast<uint64_t>(newPageTable) | PAGE_PRESENT | PAGE_NOT_BLOCK |
                    TABLE_NON_SECURE | PAGE_ACCESSED;
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

    *entry = physicalAddress.value() | PAGE_PRESENT | PAGE_NON_SECURE | PAGE_ACCESSED
            | PAGE_NO_UNPRIVILEGED_EXECUTION;
    if (!writeable) {
        *entry |= PAGE_READ_ONLY;
    }
    if (!executable) {
        /* in the bootloader we never map pages that will be executed in EL0 */
        *entry |= PAGE_NO_PRIVILEGED_EXECUTION;
    }
    if (!virtualAddress.isKernel()) {
        *entry |= PAGE_NOT_GLOBAL;
    }
    if (!large) {
        *entry |= PAGE_NOT_BLOCK;
    }
    if (cacheType) {
        *entry |= cacheType << PAGE_ATTRIBUTES_INDEX_SHIFT;
    }
}

hos_v1::PagingContext GetPagingContext() {
    return {
        .lowRoot = reinterpret_cast<size_t>(LowPagingRoot),
        .highRoot = reinterpret_cast<size_t>(HighPagingRoot),
    };
}
