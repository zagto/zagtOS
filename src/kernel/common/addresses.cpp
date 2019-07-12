#include <common/common.hpp>
#include <paging/MasterPageTable.hpp>
#include <memory/PlatformRegions.hpp>


bool Address::isPageAligned() {
    return _value % PAGE_SIZE == 0;
}

usize Address::value() {
    return _value;
}


PhysicalAddress PhysicalAddress::fromIdentitdyMappedPointer(void *ptr) {
    usize value = reinterpret_cast<usize>(ptr);
    Assert(VirtualAddress::checkInRegion(IdentityMapping, value));

    return PhysicalAddress(value - IdentityMapping.start);
}

VirtualAddress::VirtualAddress(usize value) : Address(value) {
    checkInUsableRegion();
}

bool VirtualAddress::checkInRegion(const Region &region, usize address) {
    return address >= region.start && address < region.end();
}

bool VirtualAddress::isInRegion(const Region &region) {
    return VirtualAddress::checkInRegion(region, value());
}

bool VirtualAddress::isKernel() {
    return value() >= UserSpaceRegion.end();
}


void VirtualAddress::checkInUsableRegion() {
    for (const Region &region : AllRegions) {
        if (isInRegion(region)) {
            return;
        }
    }

    Log << "Virtual address out of range: " << value() << EndLine;
    Panic();
}


KernelVirtualAddress::KernelVirtualAddress(usize value) : VirtualAddress(value) {
    Assert(isKernel());
}


KernelVirtualAddress::KernelVirtualAddress(const void *pointer) :
    KernelVirtualAddress(reinterpret_cast<usize>(pointer)) {}


UserVirtualAddress::UserVirtualAddress(usize value) : VirtualAddress(value) {
    if (!isInRegion(UserSpaceRegion)) {
        Log << "Virtual address not in user space region: " << this->value() << EndLine;
        Panic();
    }
}

bool UserVirtualAddress::checkInRegion(usize address) {
    return VirtualAddress::checkInRegion(UserSpaceRegion, address);
}
