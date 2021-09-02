#include <interrupts/LocalAPIC.hpp>
#include <memory/KernelPageAllocator.hpp>
#include <system/System.hpp>
#include <system/Processor.hpp>

void LocalAPIC::writeRegister(Register reg, uint32_t value) {
    *reinterpret_cast<volatile uint32_t *>(map + static_cast<size_t>(reg)) = value;
}

uint32_t LocalAPIC::readRegister(Register reg) {
    return *reinterpret_cast<volatile uint32_t *>(map + static_cast<size_t>(reg));
}

Status LocalAPIC::setupMap(PhysicalAddress base) {
    assert(base.value() / PAGE_SIZE
           == (base.value() + static_cast<size_t>(Register::END) - 1) / PAGE_SIZE);
    PhysicalAddress physicalFrame = align(base.value(), PAGE_SIZE, AlignDirection::DOWN);
    scoped_lock sl(KernelPageAllocator.lock);
    Result<void *> virtualPage = KernelPageAllocator.map(PAGE_SIZE,
                                                         false,
                                                         &physicalFrame,
                                                         CacheType::NONE);
    if (!virtualPage) {
        return virtualPage.status();
    }

    map = static_cast<uint8_t *>(*virtualPage) + base.value() % PAGE_SIZE;
    return Status::OK();
}

void LocalAPIC::wirteInterruptControlRegister(DeliveryMode deliveryMode,
                                              Level level,
                                              TriggerMode triggerMode,
                                              uint32_t destination,
                                              uint8_t vector) {
    while (readRegister(Register::INTERRUPT_COMMAND_LOW) & (1u<<12)) {
        /* wait for Delivery status bit to clear */
    }
    writeRegister(Register::INTERRUPT_COMMAND_HIGH, destination << (56 - 32));

    writeRegister(Register::INTERRUPT_COMMAND_LOW,
            vector
            | (static_cast<uint32_t>(deliveryMode) << 8)
            | (static_cast<uint32_t>(level) << 14)
            | (static_cast<uint32_t>(triggerMode) << 15)
            | (readRegister(Register::INTERRUPT_COMMAND_LOW) & 0b111111111111000010000000000000u));

    while (readRegister(Register::INTERRUPT_COMMAND_LOW) & (1u<<12)) {
        /* wait for Delivery status bit to clear */
    }

}

void LocalAPIC::sendIPI(uint32_t apicID, uint8_t vector) {
    wirteInterruptControlRegister(DeliveryMode::FIXED,
                                  Level::ASSERT,
                                  TriggerMode::EDGE,
                                  apicID,
                                  vector);
    /*wirteInterruptControlRegister(DeliveryMode::FIXED,
                                  Level::DEASSERT,
                                  TriggerMode::EDGE,
                                  apicID,
                                  vector);*/
}

LocalAPIC::LocalAPIC(Status &) :
        timer(this) {
}

Status LocalAPIC::initialize(PhysicalAddress base) {
    Status status = setupMap(base);
    if (!status) {
        return status;
    }

    assert(Processor::kernelInterruptsLock.isLocked());

    cout << "Initializing local APIC on Processor" << CurrentProcessor->id << " APIC base " << base.value() << " mapped to " << map << endl;

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
    writeRegister(Register::SPURIOUS_INTERRUPT_VECTOR, 0x120);

    writeRegister(Register::LVT_REGULAR_INTTERRUPTS, 0x08040);
    writeRegister(Register::LVT_NON_MASKABLE_INTERRUPTS, 0x0040c);


    return Status::OK();
}

LocalAPIC::~LocalAPIC() {
    Panic();
}
