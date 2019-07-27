#ifndef PAGETABLEENTRY_HPP
#define PAGETABLEENTRY_HPP

#include <common/common.hpp>

class PageTable;


class PageTableEntry {
public:
private:
    static const usize ADDRESS_MASK = 0x000ffffffffff000;
    static const usize PRESENT_BIT        = 1;
    static const usize WRITEABLE_BIT      = 1 << 1;
    static const usize USER_BIT           = 1 << 2;
    static const usize GLOBAL_BIT         = 1 << 8;
    static const usize NON_EXECUTABLE_BIT = 1ul << 63;

    usize data;

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
