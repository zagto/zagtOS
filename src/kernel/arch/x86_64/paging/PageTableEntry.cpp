#include <common/common.hpp>
#include <paging/PageTableEntry.hpp>
#include <paging/PageTable.hpp>
#include <system/System.hpp>


PageTableEntry::PageTableEntry() {
    data = 0;
}

PageTableEntry::PageTableEntry(PhysicalAddress addressValue,
                               Permissions permissions,
                               bool user,
                               bool disableCache) {
    assert(addressValue.isPageAligned());

    data = PRESENT_BIT | addressValue.value();
    if (user) {
        data |= USER_BIT;
    } else {
        data |= GLOBAL_BIT;
    }

    if (disableCache) {
        data |= DISABLE_CACHE_BIT;
    }

    switch (permissions) {
    case Permissions::WRITE:
        data |= WRITEABLE_BIT | NON_EXECUTABLE_BIT;
        break;
    case Permissions::WRITE_AND_EXECUTE:
        data |= WRITEABLE_BIT;
        break;
    case Permissions::EXECUTE:
        break; // do nothing
    case Permissions::NONE:
        data |= NON_EXECUTABLE_BIT;
    }
}

bool PageTableEntry::present() {
    return data & PRESENT_BIT;
}

PhysicalAddress PageTableEntry::addressValue() {
    assert(present());

    return data & ADDRESS_MASK;
}

void PageTableEntry::setAddressValue(PhysicalAddress addressValue) {
    assert(addressValue.isPageAligned());

    data = (data & ~ADDRESS_MASK) | (addressValue.value() & ADDRESS_MASK);
}
