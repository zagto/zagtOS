#ifndef PAGETABLE_HPP
#define PAGETABLE_HPP

#include <common/common.hpp>
#include <paging/PageTableEntry.hpp>

class PageTable {
public:
    static const usize NUM_LEVELS{4};
    static const usize MASTER_LEVEL{NUM_LEVELS - 1};
    static const usize NUM_ENTRIES{PAGE_SIZE / sizeof(PageTableEntry)};

protected:
    static const usize TABLE_LEVEL_SHIFT = 9;
    PageTableEntry entries[NUM_ENTRIES];

    static usize indexFor(VirtualAddress address, usize level);

public:
    PageTableEntry *entryFor(VirtualAddress address, usize level);
    void unmapEverything(usize level);
};

#endif // PAGETABLE_HPP
