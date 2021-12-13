#pragma once

#include <common/common.hpp>
#include <mutex>
#include <interrupts/APICEnum.hpp>
#include <interrupts/Interrupts.hpp>

class BoundInterrupt;

namespace apic {

class IOAPIC {
private:
    /* actual offsets in the memory map */
    static constexpr uint32_t IOREGSEL = 0x0;
    static constexpr uint32_t IOWIN = 0x4;

    /* values for IOREGSEL to access APIC registers in IOWIN */
    static constexpr uint32_t REG_IOAPICVER = 0x1;
    static constexpr uint32_t REG_FIRST_ENTRY = 0x10;

    volatile uint32_t *map;

    SpinLock lock;
    size_t _numEntries;
    /* offset into the Global System Interrupts space, i.e. the number of a legacy x86 IRQ after
     * redirection (https://wiki.osdev.org/MADT#Entry_Type_2_:_IO.2FAPIC_Interrupt_Source_Override) */
    uint32_t gsiBase;

    uint32_t readRegister(uint32_t offset) noexcept;
    void writeRegister(uint32_t offset, uint32_t value) noexcept;

public:
    IOAPIC(PhysicalAddress physicalAddress, uint32_t gsiBase);
    IOAPIC(IOAPIC &&other) noexcept;
    ~IOAPIC() noexcept;

    size_t numEntries() noexcept;
    void bindInterrutpt(BoundInterrupt &boundInterrupt) noexcept;
    void unbindInterrutpt(BoundInterrupt &boundInterrupt) noexcept;
    void endOfLevelInterrupt() noexcept;
    bool isForGSI(uint32_t gsi) noexcept;
};

}
