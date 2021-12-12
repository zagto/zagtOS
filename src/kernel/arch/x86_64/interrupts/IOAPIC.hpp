#pragma once

#include <common/common.hpp>
#include <mutex>
#include <interrupts/APICEnum.hpp>

namespace apic {

class IOAPIC {
private:
    volatile uint32_t *map;

    uint32_t readRegister(uint32_t offset) noexcept;
    void writeRegister(uint32_t offset) noexcept;

    SpinLock lock;
    size_t _numEntries;

    /* actual offsets in the memory map */
    static constexpr uint32_t IOREGSEL = 0x0;
    static constexpr uint32_t IOWIN = 0x1;

    /* values for IOREGSEL to access APIC registers in IOWIN */
    static constexpr uint32_t REG_IOAPICVER = 0x1;
    static constexpr uint32_t REG_FIRST_ENTRY = 0x10;


public:
    IOAPIC(PhysicalAddress physicalAddress);

    size_t numEntries() noexcept;
    void writeEntry(size_t entry,
                    uint8_t vector,
                    DeliveryMode deliveryMode,
                    Polarity polarity,
                    TriggerMode triggerMode,
                    bool enabled,
                    size_t destinationID);
};

}

