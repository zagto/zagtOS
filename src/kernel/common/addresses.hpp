#pragma once

#include <common/inttypes.hpp>
#include <memory/ArchRegions.hpp>

class Address {
private:
    size_t _value;

public:
    Address(size_t value) : _value{value} {}

    size_t value() const;
    bool isPageAligned();
};

class VirtualAddress : public Address {
private:
    void checkInUsableRegion();

public:
    VirtualAddress() : Address(0) {}
    VirtualAddress(size_t value);

    static bool checkInRegion(const Region &region, size_t address);

    bool isInRegion(const Region &region) const;
    bool isKernel() const;
    bool isNull() const;
    template<typename T> T *asPointer() const {
        return reinterpret_cast<T *>(value());
    }
    bool operator==(VirtualAddress other) const {
        return value() == other.value();
    }
    bool operator!=(VirtualAddress other) const {
        return value() != other.value();
    }
};

class KernelVirtualAddress : public VirtualAddress {
public:
    KernelVirtualAddress() : VirtualAddress() {}
    KernelVirtualAddress(size_t value);
    KernelVirtualAddress(const void *pointer);

    KernelVirtualAddress operator+(size_t offset) const {
        return KernelVirtualAddress(value() + offset);
    }
    KernelVirtualAddress operator+(ssize_t offset) const {
        return KernelVirtualAddress(value() + static_cast<size_t>(offset));
    }
    KernelVirtualAddress operator-(size_t offset) const {
        return KernelVirtualAddress(value() - offset);
    }
    KernelVirtualAddress operator-(ssize_t offset) const {
        return KernelVirtualAddress(value() - static_cast<size_t>(offset));
    }
};

class PhysicalAddress : public Address {
public:
    static const size_t Null{0x1337affe1337affe};

    PhysicalAddress() : Address(Null) {}
    PhysicalAddress(size_t value) : Address(value) {}

    KernelVirtualAddress identityMapped() const {
        return KernelVirtualAddress(value() + IdentityMapping.start);
    }
    static PhysicalAddress fromIdentitdyMappedPointer(void *ptr);

    bool operator==(PhysicalAddress other) const {
        return value() == other.value();
    }
    PhysicalAddress operator+(size_t offset) const {
        return PhysicalAddress(value() + offset);
    }
};

class UserVirtualAddress : public VirtualAddress {
public:
    static bool checkInRegion(size_t address);

    UserVirtualAddress() : VirtualAddress() {}
    UserVirtualAddress(size_t value);

    UserVirtualAddress operator+(size_t offset) const {
        return UserVirtualAddress(value() + offset);
    }
    UserVirtualAddress operator+(ssize_t offset) const {
        return UserVirtualAddress(value() + static_cast<size_t>(offset));
    }
    UserVirtualAddress operator-(size_t offset) const {
        return UserVirtualAddress(value() - offset);
    }
    UserVirtualAddress operator-(ssize_t offset) const {
        return UserVirtualAddress(value() - static_cast<size_t>(offset));
    }
};
