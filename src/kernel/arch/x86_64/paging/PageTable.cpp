#include <common/common.hpp>
#include <paging/PageTable.hpp>
#include <paging/PagingContext.hpp>
#include <system/System.hpp>


size_t PageTable::indexFor(VirtualAddress address, size_t level) {
    assert(address.isPageAligned());
    assert(level <= PagingContext::MASTER_LEVEL);

    static const size_t ADDRESS_BITS = (size_t(1) << 48) - 1;
    static const size_t INDEX_MASK = ((size_t(1)) << TABLE_LEVEL_SHIFT) - 1;

    return ((address.value() & ADDRESS_BITS) >> (12 + level * TABLE_LEVEL_SHIFT)) & INDEX_MASK;
}


PageTableEntry *PageTable::entryFor(VirtualAddress address, size_t level) {
    return &entries[indexFor(address, level)];
}

void PageTable::unmapEverything(size_t level) {
    for (size_t index = 0; index < NUM_ENTRIES; index++) {
        PageTableEntry &entry = entries[index];
        if (entry.present()) {
            if (level > 0) {
                entry.addressValue().identityMapped().asPointer<PageTable>()->unmapEverything(level - 1);
            }
            CurrentSystem.memory.freePhysicalFrame(entry.addressValue());
        }
    }
}
