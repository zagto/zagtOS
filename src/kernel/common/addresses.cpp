#include <common/addresses.hpp>
#include <log/Logger.hpp>
#include <memory/ArchRegions.hpp>


bool Address::isPageAligned() {
    return _value % PAGE_SIZE == 0;
}

size_t Address::value() const {
    return _value;
}

PhysicalAddress PhysicalAddress::fromIdentitdyMappedPointer(void *ptr) {
    size_t value = reinterpret_cast<size_t>(ptr);
    assert(VirtualAddress::checkInRegion(IdentityMapping, value));

    return {value - IdentityMapping.start};
}

VirtualAddress::VirtualAddress(size_t value) : Address(value) {
    checkInUsableRegion();
}

bool VirtualAddress::checkInRegion(const Region &region, size_t address) {
    return address >= region.start && address < region.end();
}

bool VirtualAddress::isInRegion(const Region &region) const {
    return VirtualAddress::checkInRegion(region, value());
}

bool VirtualAddress::isKernel() const {
    return value() >= UserSpaceRegion.end();
}

bool VirtualAddress::isNull() const {
    return value() == 0;
}


void VirtualAddress::checkInUsableRegion() {
    for (const Region &region : AllRegions) {
        if (isInRegion(region)) {
            return;
        }
    }

    cout << "Virtual address out of range: " << value() << endl;
    Panic();
}


KernelVirtualAddress::KernelVirtualAddress(size_t value) : VirtualAddress(value) {
    assert(value);
    assert(isKernel());
}


KernelVirtualAddress::KernelVirtualAddress(const void *pointer) :
    KernelVirtualAddress(reinterpret_cast<size_t>(pointer)) {}


UserVirtualAddress::UserVirtualAddress(size_t value) : VirtualAddress(value) {
    if (!isInRegion(UserSpaceRegion)) {
        cout << "Virtual address not in user space region: " << this->value() << endl;
        Panic();
    }
}

bool UserVirtualAddress::checkInRegion(size_t address) {
    return VirtualAddress::checkInRegion(UserSpaceRegion, address);
}
