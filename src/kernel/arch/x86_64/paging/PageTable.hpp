#ifndef PAGETABLE_HPP
#define PAGETABLE_HPP

#include <common/common.hpp>
#include <paging/PageTableEntry.hpp>

class PageTable {
public:
    static const size_t NUM_LEVELS{4};
    static const size_t MASTER_LEVEL{NUM_LEVELS - 1};
    static const size_t NUM_ENTRIES{PAGE_SIZE / sizeof(PageTableEntry)};

protected:
    static const size_t TABLE_LEVEL_SHIFT = 9;
    PageTableEntry entries[NUM_ENTRIES];

    static size_t indexFor(VirtualAddress address, size_t level);

public:
    PageTableEntry *entryFor(VirtualAddress address, size_t level);
    void unmapEverything(size_t level);
};

#endif // PAGETABLE_HPP
