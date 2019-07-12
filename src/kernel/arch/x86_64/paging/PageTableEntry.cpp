#include <common/common.hpp>
#include <paging/PageTableEntry.hpp>
#include <paging/PageTable.hpp>
#include <system/System.hpp>


PageTableEntry::PageTableEntry() {
    data = 0;
}

PageTableEntry::PageTableEntry(PhysicalAddress addressValue,
                               Permissions permissions,
                               bool user) {
    Assert(addressValue.isPageAligned());

    data = PRESENT_BIT | addressValue.value();
    if (user) {
        data |= USER_BIT;
    } else {
        data |= GLOBAL_BIT;
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

PageTable *PageTableEntry::pageTable() {
    usize address = reinterpret_cast<usize>(this);
    address -= address % PAGE_SIZE;
    return reinterpret_cast<PageTable *>(address);
}

usize PageTableEntry::index() {
    usize ownAddress = reinterpret_cast<usize>(this);
    return ownAddress % PAGE_SIZE;
}

bool PageTableEntry::pointsToPageTable() {
    return pageTable()->level() > 0;
}

PhysicalAddress PageTableEntry::addressValue() {
    Assert(present());

    return data & ADDRESS_MASK;
}

void PageTableEntry::setAddressValue(PhysicalAddress addressValue) {
    Assert(addressValue.isPageAligned());

    data = (data & ~ADDRESS_MASK) | (addressValue.value() & ADDRESS_MASK);
}
