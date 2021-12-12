#include <interrupts/IOAPIC.hpp>
#include <memory/KernelPageAllocator.hpp>

namespace apic {

uint32_t IOAPIC::readRegister(uint32_t offset) noexcept {
    map[IOREGSEL] = offset;
    return map[IOWIN];
}

IOAPIC::IOAPIC(PhysicalAddress physicalAddress, uint32_t gsiBase) :
        gsiBase{gsiBase} {

    assert(physicalAddress.isPageAligned());

    {
        scoped_lock sl(KernelPageAllocator.lock);
        void *result = KernelPageAllocator.map(PAGE_SIZE, false, &physicalAddress, CacheType::NONE);
        map = static_cast<volatile uint32_t *>(result);
    }

    uint32_t versionRegister = readRegister(REG_IOAPICVER);
    /* number of entries is in bits 16-23 of version register (minus 1) */
    _numEntries = ((versionRegister >> 16) & 0xff) + 1;
}

IOAPIC::IOAPIC(IOAPIC &&other) noexcept :
    map{other.map},
    _numEntries{other.numEntries()},
    gsiBase{other.gsiBase} {

    /* These objects should not be moved around while someone uses them */
    assert(!other.lock.isLocked());

    other._numEntries = 0;
    other.map = nullptr;
    other.gsiBase = 0;
}

IOAPIC::~IOAPIC() noexcept {
    if (map) {
        scoped_lock sl(KernelPageAllocator.lock);
        void *voidMap = reinterpret_cast<void *>(reinterpret_cast<size_t>(map));
        KernelPageAllocator.unmap(voidMap, PAGE_SIZE, false);
    }
}

size_t IOAPIC::numEntries() noexcept {
    return _numEntries;
}

}
