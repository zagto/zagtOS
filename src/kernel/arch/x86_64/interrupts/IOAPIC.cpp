#include <interrupts/IOAPIC.hpp>
#include <memory/KernelPageAllocator.hpp>
#include <processes/InterruptManager.hpp>

namespace apic {

uint32_t IOAPIC::readRegister(uint32_t offset) noexcept {
    map[IOREGSEL] = offset;
    return map[IOWIN];
}

void IOAPIC::writeRegister(uint32_t offset, uint32_t value) noexcept {
    map[IOREGSEL] = offset;
    map[IOWIN] = value;
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

void IOAPIC::bindInterrutpt(BoundInterrupt &boundInterrupt) noexcept {
    assert(isForGSI(boundInterrupt.typeData));
    scoped_lock sl(lock);

    uint32_t low = static_cast<uint32_t>(boundInterrupt.processorInterrupt.vectorNumber)
            | (static_cast<uint32_t>(DeliveryMode::FIXED) << 8)
            | (boundInterrupt.triggerMode.x86Polarity() << 13)
            | (boundInterrupt.triggerMode.x86TriggerMode() << 15);
    uint32_t high = (boundInterrupt.processorInterrupt.processorID << 24)
            | 1 /* enabled */;

    assert(boundInterrupt.processorInterrupt.processorID < 0x10); // TODO

    writeRegister(REG_FIRST_ENTRY + 2 * (boundInterrupt.typeData - gsiBase), low);
    writeRegister(REG_FIRST_ENTRY + 2 * (boundInterrupt.typeData - gsiBase) + 1, high);
}

void IOAPIC::unbindInterrutpt(BoundInterrupt &boundInterrupt) noexcept {
    assert(isForGSI(boundInterrupt.typeData));
    scoped_lock sl(lock);

    writeRegister(REG_FIRST_ENTRY + 2 * (boundInterrupt.typeData - gsiBase), 0);
    writeRegister(REG_FIRST_ENTRY + 2 * (boundInterrupt.typeData - gsiBase) + 1, 0);
}

void IOAPIC::endOfLevelInterrupt() noexcept {
    cout << "TODO: IOAPIC EOI" << endl;
    Panic();
}

bool IOAPIC::isForGSI(uint32_t gsi) noexcept {
    return gsi >= gsiBase && gsi < gsiBase + _numEntries;
}

}
