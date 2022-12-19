#pragma once
#include <common/common.hpp>

class PageTable;

enum class CacheType {
    NORMAL_WRITE_BACK, NONE, WRITE_THROUGH, WRITE_COMBINING
};

class PageTableEntry {
private:
    friend class PageTable;

    static constexpr uint64_t ADDRESS_MASK = 0x0007fffffffff000;
    static constexpr uint64_t PAGE_PRESENT = 1;
    /* not a block in terms of ARM paging, i.e. not a large page */
    static constexpr uint64_t PAGE_NOT_BLOCK = 1ul << 1;
    static constexpr uint64_t PAGE_ATTRIBUTES_INDEX_SHIFT = 2;
    static constexpr uint64_t PAGE_NON_SECURE = (1ul << 5);
    static constexpr uint64_t PAGE_USER = (1ul << 6);
    static constexpr uint64_t PAGE_READ_ONLY = (1ul << 7);
    static constexpr uint64_t PAGE_ACCESSED = (1ul << 10);
    static constexpr uint64_t PAGE_OUTER_SHARABLE = (0b10ul << 8);
    static constexpr uint64_t PAGE_INNER_SHARABLE = (0b11ul << 8);
    static constexpr uint64_t PAGE_NOT_GLOBAL = (1ul<<11);

    static constexpr uint64_t PAGE_NO_PRIVILEGED_EXECUTION = (1ul << 53);
    static constexpr uint64_t PAGE_NO_UNPRIVILEGED_EXECUTION = (1ul << 54);

    static constexpr uint64_t TABLE_NON_SECURE = (1ul << 63);
    uint64_t data;

    void setPermissions(Permissions newPermissions);

public:
    PageTableEntry(uint64_t data = 0);
    PageTableEntry(PhysicalAddress addressValue,
                   Permissions permissions,
                   bool lastLevel,
                   bool user,
                   CacheType cacheType);

    bool present();
    PhysicalAddress addressValue();
    void setAddressValue(PhysicalAddress addressValue);
};

