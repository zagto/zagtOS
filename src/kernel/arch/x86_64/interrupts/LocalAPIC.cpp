#include <interrupts/LocalAPIC.hpp>
#include <memory/KernelPageAllocator.hpp>
#include <system/System.hpp>
#include <system/Processor.hpp>

namespace apic {

void LocalAPIC::writeRegister(Register reg, uint32_t value) noexcept {
    *reinterpret_cast<volatile uint32_t *>(map + static_cast<size_t>(reg)) = value;
}

uint32_t LocalAPIC::readRegister(Register reg) noexcept {
    return *reinterpret_cast<volatile uint32_t *>(map + static_cast<size_t>(reg));
}

void LocalAPIC::setupMap(PhysicalAddress base) {
    assert(base.value() / PAGE_SIZE
           == (base.value() + static_cast<size_t>(Register::END) - 1) / PAGE_SIZE);
    PhysicalAddress physicalFrame = align(base.value(), PAGE_SIZE, AlignDirection::DOWN);
    scoped_lock sl(KernelPageAllocator.lock);
    void *virtualPage = KernelPageAllocator.map(PAGE_SIZE,
                                                false,
                                                &physicalFrame,
                                                CacheType::NONE);

    map = static_cast<uint8_t *>(virtualPage) + base.value() % PAGE_SIZE;
}

void LocalAPIC::writeInterruptControlRegister(DeliveryMode deliveryMode,
                                              Level level,
                                              TriggerMode triggerMode,
                                              uint32_t destination,
                                              uint8_t vector) noexcept {
    while (readRegister(Register::INTERRUPT_COMMAND_LOW) & (1u<<12)) {
        /* wait for Delivery status bit to clear */
    }
    writeRegister(Register::INTERRUPT_COMMAND_HIGH, destination << (56 - 32));

    writeRegister(Register::INTERRUPT_COMMAND_LOW,
            vector
            | (static_cast<uint32_t>(deliveryMode) << 8)
            | (static_cast<uint32_t>(level) << 14)
            | (triggerMode.x86TriggerMode() << 15)
            | (readRegister(Register::INTERRUPT_COMMAND_LOW) & 0b111111111111000010000000000000u));

    while (readRegister(Register::INTERRUPT_COMMAND_LOW) & (1u<<12)) {
        /* wait for Delivery status bit to clear */
    }

}

void LocalAPIC::sendIPI(uint32_t apicID) noexcept {
    writeInterruptControlRegister(DeliveryMode::FIXED,
                                  Level::ASSERT,
                                  TriggerMode::RISING_EDGE,
                                  apicID,
                                  static_cast<uint32_t>(StaticInterrupt::IPI));
}

void LocalAPIC::endOfInterrupt() noexcept {
    writeRegister(Register::END_OF_INTERRUPT, 0);
}


void LocalAPIC::initialize(PhysicalAddress base) {
    setupMap(base);

    assert(KernelInterruptsLock.isLocked());

    /* initialization sequence from:
     * http://www.osdever.net/tutorials/view/advanced-programming-interrupt-controller */
    writeRegister(Register::TASK_PRIORITY, 0x20);
    writeRegister(Register::LVT_TIMER, 0x10000);
    writeRegister(Register::LVT_PERFORMANCE_COUNTER, 0x10000);
    writeRegister(Register::LVT_REGULAR_INTTERRUPTS, 0x10000);
    writeRegister(Register::LVT_NON_MASKABLE_INTERRUPTS, 0x10000);
    writeRegister(Register::LVT_ERROR, 0x10000);

    /* enables the APIC and sets spurious interrupts vector.
     * make spurious interrupts use 0x20 the same vector they use on the legacy PIC, so we can
     * ignore them all the same way */
    writeRegister(Register::SPURIOUS_INTERRUPT_VECTOR,
                  static_cast<uint32_t>(StaticInterrupt::APIC_SPURIOUS) | 0x100);

    writeRegister(Register::LVT_REGULAR_INTTERRUPTS, 0x08040);
    writeRegister(Register::LVT_NON_MASKABLE_INTERRUPTS, 0x0040c);

    /* initialize timer */
    constexpr int32_t TIMER_TSC_DEADLINE = 1u<<18;
    writeRegister(LocalAPIC::Register::DIVIDE_CONFIGURATION, 0);
    writeRegister(LocalAPIC::Register::INITIAL_COUNT, 0);
    writeRegister(
                LocalAPIC::Register::LVT_TIMER,
                TIMER_TSC_DEADLINE | static_cast<int32_t>(StaticInterrupt::TIMER));
}

LocalAPIC::~LocalAPIC() {
    Panic();
}

}
