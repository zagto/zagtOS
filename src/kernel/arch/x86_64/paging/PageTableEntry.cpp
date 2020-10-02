#include <common/common.hpp>
#include <paging/PageTableEntry.hpp>
#include <paging/PageTable.hpp>


PageTableEntry::PageTableEntry() {
    data = 0;
}

PageTableEntry::PageTableEntry(PhysicalAddress addressValue,
                               Permissions permissions,
                               bool user,
                               CacheType cacheType) {
    assert(addressValue.isPageAligned());

    data = PRESENT_BIT | addressValue.value();
    if (user) {
        data |= USER_BIT;
    } else {
        data |= GLOBAL_BIT;
    }

    data |= static_cast<size_t>(cacheType) << CACHE_TYPE_SHIFT;

    setPermissions(permissions);
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

void PageTableEntry::setPermissions(Permissions newPermissions) {
    switch (newPermissions) {
    case Permissions::READ_WRITE:
        data |= WRITEABLE_BIT | NON_EXECUTABLE_BIT;
        break;
    case Permissions::READ_WRITE_EXECUTE:
        data |= WRITEABLE_BIT;
        break;
    case Permissions::READ_EXECUTE:
        break; // do nothing
    case Permissions::READ:
        data |= NON_EXECUTABLE_BIT;
        break;
    default:
        cout << "invalid permissions for PageTableEntry" << endl;
        Panic();
    }
}
