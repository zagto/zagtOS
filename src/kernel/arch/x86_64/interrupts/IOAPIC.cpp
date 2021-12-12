#include <interrupts/IOAPIC.hpp>
#include <memory/KernelPageAllocator.hpp>

namespace apic {

uint32_t IOAPIC::readRegister(uint32_t offset) noexcept {
    map[IOREGSEL] = offset;
    return map[IOWIN];
}

IOAPIC::IOAPIC(PhysicalAddress physicalAddress) {
    assert(physicalAddress.isPageAligned());

    void *result = KernelPageAllocator.map(PAGE_SIZE, false, &physicalAddress, CacheType::NONE);
    map = static_cast<volatile uint32_t *>(result);

    uint32_t versionRegister = readRegister(REG_IOAPICVER);
    /* number of entries is in bits 16-23 of version register (minus 1) */
    _numEntries = ((versionRegister >> 16) & 0xff) + 1;
}

}
