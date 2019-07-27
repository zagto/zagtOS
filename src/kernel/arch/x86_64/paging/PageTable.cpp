#include <common/common.hpp>
#include <paging/PageTable.hpp>
#include <system/System.hpp>


usize PageTable::indexFor(VirtualAddress address, usize level) {
    Assert(address.isPageAligned());
    Assert(level <= MASTER_LEVEL);

    static const usize ADDRESS_BITS = (usize(1) << 48) - 1;
    static const usize INDEX_MASK = ((usize(1)) << TABLE_LEVEL_SHIFT) - 1;

    return ((address.value() & ADDRESS_BITS) >> (12 + level * TABLE_LEVEL_SHIFT)) & INDEX_MASK;
}


PageTableEntry *PageTable::entryFor(VirtualAddress address, usize level) {
    return &entries[indexFor(address, level)];
}

void PageTable::unmapEverything(usize level) {
    for (usize index = 0; index < NUM_ENTRIES; index++) {
        PageTableEntry &entry = entries[index];
        if (entry.present()) {
            if (level > 0) {
                entry.addressValue().identityMapped().asPointer<PageTable>()->unmapEverything(level - 1);
            }
            //CurrentSystem.memory.freePhysicalFrame(entry.addressValue());
        }
    }
}
