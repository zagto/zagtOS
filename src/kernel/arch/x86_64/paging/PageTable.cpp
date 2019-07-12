#include <common/common.hpp>
#include <paging/PageTable.hpp>


usize PageTable::level() {
    usize ownAddress = reinterpret_cast<usize>(this);
    usize limit = 0;

    for (usize level = 0; level < NUM_LEVELS; level++) {
        limit -= (1 << level * TABLE_LEVEL_SHIFT) * PAGE_SIZE;
        if (ownAddress >= limit) {
            return level;
        }
    }

    Log << "called level on what is not an active page table\n";
    Panic();
}

bool PageTable::isMaster() {
    return level() == NUM_LEVELS - 1;
}


PageTableEntry *PageTable::entryFor(VirtualAddress address, usize level) {
    Assert(address.isPageAligned());
    Assert(level >= 1 && level <= 4);

    static const usize ADDRESS_BITS = (usize(1) << 48) - 1;
    static const usize INDEX_MASK = ((usize(1)) << 12) - 1;

    return &entries[((address.value() & ADDRESS_BITS) >> ((3 + level * 9))) & INDEX_MASK];
}
