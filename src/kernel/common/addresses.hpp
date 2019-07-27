#ifndef ADDRESSES_HPP
#define ADDRESSES_HPP

#include <common/inttypes.hpp>
#include <memory/PlatformRegions.hpp>

class Address {
private:
    size_t _value;

public:
    Address(size_t value) : _value{value} {}

    size_t value();
    bool isPageAligned();
};

class VirtualAddress : public Address {
private:
    void checkInUsableRegion();

public:
    VirtualAddress() : Address(0) {}
    VirtualAddress(size_t value);

    static bool checkInRegion(const Region &region, size_t address);

    bool isInRegion(const Region &region);
    bool isKernel();
    template<typename T> T *asPointer() {
        return reinterpret_cast<T *>(value());
    }
};

class KernelVirtualAddress : public VirtualAddress {
public:
    KernelVirtualAddress() : VirtualAddress() {}
    KernelVirtualAddress(size_t value);
    KernelVirtualAddress(const void *pointer);

    KernelVirtualAddress operator+(size_t offset) {
        return KernelVirtualAddress(value() + offset);
    }
    KernelVirtualAddress operator+(ssize_t offset) {
        return KernelVirtualAddress(value() + offset);
    }

};

class PhysicalAddress : public Address {
public:
    static const size_t NULL{0x1337affe1337affe};

    PhysicalAddress() : Address(NULL) {}
    PhysicalAddress(size_t value) : Address(value) {}

    KernelVirtualAddress identityMapped() {
        return KernelVirtualAddress(value() + IdentityMapping.start);
    }
    static PhysicalAddress fromIdentitdyMappedPointer(void *ptr);

    bool operator==(PhysicalAddress other) {
        return value() == other.value();
    }
    PhysicalAddress operator+(size_t offset) {
        return PhysicalAddress(value() + offset);
    }
};

class UserVirtualAddress : public VirtualAddress {
public:
    static bool checkInRegion(size_t address);

    UserVirtualAddress() : VirtualAddress() {}
    UserVirtualAddress(size_t value);
};

#endif
