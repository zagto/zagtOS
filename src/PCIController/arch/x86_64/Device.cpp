#include <Device.hpp>

using namespace zagtos;

#ifdef __x86_64__

Interrupt Device::allocateMSIInterrupt() {
    ProcessorInterruptInfo info;
    Interrupt interrupt(zagtos::TriggerMode::RISING_EDGE, info);

    /* see: https://wiki.osdev.org/PCI#Enabling_MSI */
    static constexpr uint32_t EDGE_TRIGGER = 1u << 15;
    static constexpr uint32_t ACTIVE_HIGH = 1u << 14;

    uint64_t address = 0xfee00000 | (info.processorID << 12);
    assert(info.vectorNumber <= 0xff);
    uint32_t data = info.vectorNumber | EDGE_TRIGGER | ACTIVE_HIGH;

    assert(MSI != nullptr);
    if (MSI->header.is64bit()) {
        auto capability = static_cast<MSICapability64 *>(MSI);
        capability->addressLow(static_cast<uint32_t>(address));
        capability->addressHigh(static_cast<uint32_t>(address >> 32));
        capability->data(data);
        capability->mask(0);
        capability->header.enable(1);
    } else {
        auto capability = static_cast<MSICapability32 *>(MSI);
        capability->address(static_cast<uint32_t>(address));
        capability->data(data);
        capability->mask(0);
        capability->header.enable(1);
    }
    return interrupt;
}

#endif
