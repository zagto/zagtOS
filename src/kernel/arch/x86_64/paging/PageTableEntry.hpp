#pragma once
#include <common/common.hpp>

class PageTable;

enum class CacheType {
    NORMAL_WRITE_BACK, NONE, WRITE_THROUGH, WRITE_COMBINING
};

class PageTableEntry {
public:
private:
    static const size_t ADDRESS_MASK = 0x000ffffffffff000;
    static const size_t PRESENT_BIT        = 1;
    static const size_t WRITEABLE_BIT      = 1 << 1;
    static const size_t USER_BIT           = 1 << 2;
    static const size_t CACHE_TYPE_SHIFT   = 3;
    static const size_t GLOBAL_BIT         = 1 << 8;
    static const size_t NON_EXECUTABLE_BIT = 1ul << 63;

    size_t data;

public:
    PageTableEntry();
    PageTableEntry(PhysicalAddress addressValue,
                   Permissions permissions,
                   bool user,
                   CacheType cacheType);

    bool present();
    PhysicalAddress addressValue();
    void setAddressValue(PhysicalAddress addressValue);
    void setPermissions(Permissions newPermissions);
};

