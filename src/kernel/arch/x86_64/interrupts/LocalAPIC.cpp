#include <interrupts/LocalAPIC.hpp>
#include <memory/Memory.hpp>
#include <system/System.hpp>
#include <paging/PagingContext.hpp>

void LocalAPIC::writeRegister(Register reg, uint32_t value) {
    *reinterpret_cast<volatile uint32_t *>(map + static_cast<size_t>(reg)) = value;
}

uint32_t LocalAPIC::readRegister(Register reg) {
    return *reinterpret_cast<volatile uint32_t *>(map + static_cast<size_t>(reg));
}

void LocalAPIC::setupMap(PhysicalAddress base) {
    /* ugly way to map the APIC into kernel memory.
     * since this is currently the only memory-mapped device the kernel uses directly, this works
     * ok for now.
     * TODO: build a genenerec way the kernel accesses memory-mapped devices */
    assert(base.value() / PAGE_SIZE == (base.value() + static_cast<size_t>(Register::END) - 1) / PAGE_SIZE);
    PhysicalAddress apicFrame{align(base.value(), PAGE_SIZE, AlignDirection::DOWN)};

    KernelVirtualAddress mapAddress = CurrentSystem.memory.allocateVirtualArea(PAGE_SIZE,
                                                    PAGE_SIZE);
    PhysicalAddress physical = PagingContext::resolve(mapAddress);
    PagingContext::unmap(mapAddress);
    CurrentSystem.memory.freePhysicalFrame(physical);
    PagingContext::map(mapAddress, apicFrame, Permissions::WRITE, true);
    PagingContext::invalidateLocally(mapAddress);
    map = mapAddress.asPointer<uint8_t>();
}

LocalAPIC::LocalAPIC(PhysicalAddress base) :
        timer(this) {

    setupMap(base);

    /* initialization sequence from:
     * http://www.osdever.net/tutorials/view/advanced-programming-interrupt-controller */
    writeRegister(Register::TASK_PRIORITY, 0x20);
    writeRegister(Register::LVT_TIMER, 0x10000);
    writeRegister(Register::LVT_PERFORMANCE_COUNTER, 0x10000);
    writeRegister(Register::LVT_REGULATOR_INTTERRUPTS, 0x08700);
    writeRegister(Register::LVT_NON_MASKABLE_INTERRUPTS, 0x00400);
    writeRegister(Register::LVT_ERROR, 0x10000);

    /* enables the APIC and sets spurious interrupts vector.
     * make spurious interrupts use 0x20 the same vector they use on the legacy PIC, so we can
     * ignore them all the same way */
    writeRegister(Register::SPURIOUS_INTERRUPT_VECOTOR, 0x120);

    writeRegister(Register::LVT_REGULATOR_INTTERRUPTS, 0x08700);
    writeRegister(Register::LVT_NON_MASKABLE_INTERRUPTS, 0x00400);
}


LocalAPIC::~LocalAPIC() {
    Panic();
}


void LocalAPIC::sendInit(uint32_t apicID) {
    wirteInterruptControlRegister(DeliveryMode::INIT,
                                  Level::ASSERT,
                                  TriggerMode::LEVEL,
                                  apicID,
                                  0);
    wirteInterruptControlRegister(DeliveryMode::INIT,
                                  Level::DEASSERT,
                                  TriggerMode::LEVEL,
                                  apicID,
                                  0);

}

void LocalAPIC::sendStartup(uint32_t apicID, PhysicalAddress entry) {
    assert(entry.isPageAligned());
    assert(entry.value() <= 0xff * PAGE_SIZE);
    uint8_t vector = static_cast<uint8_t>(entry.value() / PAGE_SIZE);

    wirteInterruptControlRegister(DeliveryMode::STARTUP,
                                  Level::ASSERT,
                                  TriggerMode::LEVEL,
                                  apicID,
                                  vector);
    wirteInterruptControlRegister(DeliveryMode::STARTUP,
                                  Level::DEASSERT,
                                  TriggerMode::LEVEL,
                                  apicID,
                                  vector);
}

void LocalAPIC::wirteInterruptControlRegister(DeliveryMode deliveryMode,
                                              Level level,
                                              TriggerMode triggerMode,
                                              uint32_t destination,
                                              uint8_t vector) {
    writeRegister(Register::INTERRUPT_COMMAND_HIGH, destination << (56 - 32));
    writeRegister(Register::INTERRUPT_COMMAND_LOW,
            vector
            | (static_cast<uint32_t>(deliveryMode) << 8)
            | (static_cast<uint32_t>(level) << 14)
            | (static_cast<uint32_t>(triggerMode) << 15));
}
