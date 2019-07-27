#ifndef PAGETABLEENTRY_HPP
#define PAGETABLEENTRY_HPP

#include <common/common.hpp>

class PageTable;


class PageTableEntry {
public:
private:
    static const size_t ADDRESS_MASK = 0x000ffffffffff000;
    static const size_t PRESENT_BIT        = 1;
    static const size_t WRITEABLE_BIT      = 1 << 1;
    static const size_t USER_BIT           = 1 << 2;
    static const size_t GLOBAL_BIT         = 1 << 8;
    static const size_t NON_EXECUTABLE_BIT = 1ul << 63;

    size_t data;

public:
    PageTableEntry();
    PageTableEntry(PhysicalAddress addressValue,
                   Permissions permissions,
                   bool user);

    bool present();
    PhysicalAddress addressValue();
    void setAddressValue(PhysicalAddress addressValue);
};

#endif // PAGETABLEENTRY_HPP
