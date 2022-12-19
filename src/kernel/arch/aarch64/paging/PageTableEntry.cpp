#include <common/common.hpp>
#include <paging/PageTableEntry.hpp>
#include <paging/PageTable.hpp>
#include <iostream>


PageTableEntry::PageTableEntry(size_t data):
    data{data} {}

PageTableEntry::PageTableEntry(PhysicalAddress addressValue,
                               Permissions permissions,
                               bool lastLevel,
                               bool user,
                               CacheType cacheType) {
    assert(addressValue.isPageAligned());

    data = PAGE_PRESENT | PAGE_NOT_BLOCK | PAGE_ACCESSED | addressValue.value();

    if (lastLevel) {
        data |= PAGE_NON_SECURE;
        if (user) {
            data |= PAGE_USER | PAGE_NO_PRIVILEGED_EXECUTION | PAGE_NOT_GLOBAL;
        }
        setPermissions(permissions);
        data |= static_cast<uint64_t>(cacheType) << PAGE_ATTRIBUTES_INDEX_SHIFT;
    } else {
        data |= TABLE_NON_SECURE;
    }
}

bool PageTableEntry::present() {
    return data & PAGE_PRESENT;
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
        data |= PAGE_NO_UNPRIVILEGED_EXECUTION | PAGE_NO_PRIVILEGED_EXECUTION;
        break;
    case Permissions::READ_EXECUTE:
        data |= PAGE_READ_ONLY;
        break;
    case Permissions::READ:
        data |= PAGE_READ_ONLY | PAGE_NO_UNPRIVILEGED_EXECUTION | PAGE_NO_PRIVILEGED_EXECUTION;
        break;
    default:
        cout << "invalid permissions for PageTableEntry" << endl;
        Panic();
    }
}
