#ifndef ADDRESSES_HPP
#define ADDRESSES_HPP

#include <common/inttypes.hpp>
#include <memory/PlatformRegions.hpp>

class Address {
private:
    usize _value;

public:
    Address(usize value) : _value{value} {}

    usize value();
    bool isPageAligned();
};

class VirtualAddress : public Address {
private:
    void checkInUsableRegion();

public:
    VirtualAddress() : Address(0) {}
    VirtualAddress(usize value);

    static bool checkInRegion(const Region &region, usize address);

    bool isInRegion(const Region &region);
    bool isKernel();
    template<typename T> T *asPointer() {
        return reinterpret_cast<T *>(value());
    }
};

class KernelVirtualAddress : public VirtualAddress {
public:
    KernelVirtualAddress() : VirtualAddress() {}
    KernelVirtualAddress(usize value);
    KernelVirtualAddress(const void *pointer);

    KernelVirtualAddress operator+(usize offset) {
        return KernelVirtualAddress(value() + offset);
    }
    KernelVirtualAddress operator+(isize offset) {
        return KernelVirtualAddress(value() + offset);
    }

};

class PhysicalAddress : public Address {
public:
    static const usize NULL{0x1337affe1337affe};

    PhysicalAddress() : Address(NULL) {}
    PhysicalAddress(usize value) : Address(value) {}

    KernelVirtualAddress identityMapped() {
        return KernelVirtualAddress(value() + IdentityMapping.start);
    }
    static PhysicalAddress fromIdentitdyMappedPointer(void *ptr);

    bool operator==(PhysicalAddress other) {
        return value() == other.value();
    }
    PhysicalAddress operator+(usize offset) {
        return PhysicalAddress(value() + offset);
    }
};

class UserVirtualAddress : public VirtualAddress {
public:
    static bool checkInRegion(usize address);

    UserVirtualAddress() : VirtualAddress() {}
    UserVirtualAddress(usize value);
};

#endif
